# ESP-IDF Event System Bootcamp

This document is meant to build a real mental model, not just help you copy Wi-Fi code.

The goal is that by the end you should understand:

- what an event system is in general
- what problem it solves
- how ESP-IDF's event system works
- how Wi-Fi uses it
- how to create your own events
- when to use it and when not to

Read it in order. The sections build on each other.

## 1. Why Event Systems Exist

An event system exists because some things do not happen immediately.

That is the deepest reason.

If everything happened right now, direct function calls would be enough.

Example:

- you call `add(2, 3)`
- it returns `5`

Nothing asynchronous happened. No event system needed.

Now compare that to Wi-Fi:

- you ask the ESP32 to start Wi-Fi
- hardware starts up
- radio scanning may happen
- authentication may happen
- association may happen
- DHCP may happen
- you might get disconnected later

That is not one instant operation. It is a process that unfolds over time.

So there are two basic ways to handle that:

1. Keep asking over and over: "Has it happened yet?"
2. Ask the system to notify you when it happens.

Option 2 is the event model.

That is the first mental model:

An event system is a notification system for things that happen later.

## 2. The Simplest Possible Mental Model

At the highest level, an event system has four ideas:

- a producer
- an event
- a listener
- a callback

### Producer

The producer is the code that says something happened.

Examples:

- Wi-Fi stack says "station started"
- network stack says "got IP"
- your sensor module says "measurement ready"

### Event

The event is the thing being announced.

Examples:

- `WIFI_EVENT_STA_START`
- `WIFI_EVENT_STA_DISCONNECTED`
- `IP_EVENT_STA_GOT_IP`

### Listener

The listener is code that says:

"I care when that event happens."

### Callback

The callback is the function that runs when a matching event arrives.

It is natural here to ask whether each listener gets its own callback per event. That is a good first approximation, but the real rule is slightly broader: a listener registration maps some event identity, or even a whole group of related events, to a handler function. Different listeners can absolutely use different handler functions for the same event. If ten parts of the app care about `WIFI_EVENT_STA_DISCONNECTED`, they can each register their own handler. But one listener does not have to use one separate function per event. A listener can register one handler for one specific event, or one broader handler for many events and then use `if` or `switch` inside that handler.


So the overall flow is:

1. producer posts an event
2. event system receives it
3. event system finds listeners that care
4. event system calls their callbacks

That is the whole idea.

## 3. Event Systems Versus Direct Function Calls

It is important to understand what event systems buy you.

With a direct function call:

- caller knows callee
- caller decides exactly what function to run
- relationship is tightly coupled

Example:

```c
wifi_manager_on_connected();
```

That is simple and great when the relationship is simple.

But now suppose many parts of the app might care:

- logger
- UI
- reconnect manager
- application state manager

If Wi-Fi directly calls them all, Wi-Fi becomes tangled to app code.

An event system solves that by separating:

- "something happened"
from
- "who wants to react to it"

So the producer does not need to know the listeners.

That is the key design benefit:

decoupling.

## 4. What ESP-IDF Chooses To Model As Events

ESP-IDF uses events for things like:

- Wi-Fi status changes
- IP changes
- Ethernet events
- user-defined asynchronous notifications

This is a good fit because these are control-flow events, not high-rate data streams.

That distinction matters.

Event systems are usually great for:

- state changes
- lifecycle changes
- notifications
- "this happened" signals

They are usually not the right tool for:

- very high-rate sample streams
- raw packet processing pipelines
- large bulk data transfer

So ESP-IDF events are mostly for control-plane communication, not data-plane throughput.

## 5. What Makes ESP-IDF's Event System Specific

ESP-IDF does not model an event as:

- event name
- callback pointer

Instead, ESP-IDF separates:

- event identity
- registered handlers

This is important.

The posted event does not carry the callback inside it.

Instead:

- code first registers callbacks
- later events are posted
- the event loop matches events to handlers 

This is also the place where people often start imagining that each event instance carries a linked list of callbacks around with it. That is not quite right. The linked-list part is closer to a registry owned by the event system itself. Registrations are stored in internal linked structures, and when an event arrives the event loop walks the registrations that match. So do not picture the event object carrying its own callback list. Picture the event system owning a listener registry and consulting it during dispatch.

Another common question at this point is what determines which callback runs first, especially if many listeners register for the same event. The important answer is that handlers are not chosen by FreeRTOS task priority. The event loop task has one task priority, but inside that task it dispatches handlers in the order the event system walks its registrations. For normal application design, the safest rule is: do not rely on handler order unless you have intentionally designed around registration order. If one callback is "the important one" and must definitely happen before others, that is usually a sign that those parts should not be modeled as independent listeners to the same event. Make that dependency explicit in code instead.

That means ESP-IDF is not a "run this function later" system.

It is a "publish that this happened" system.

That design allows:

- multiple listeners
- loose coupling
- reusable subsystem code

At this point it helps to clear up a naming issue: in this document, "handler" and "callback" are basically the same thing. ESP-IDF usually uses the word "handler." So when you register `myHandler`, you are registering the callback function `myHandler`.

The more interesting question is granularity. Should you register something like `onXButtonPress()` for each specific event, or a broader `onInputEvent()` and switch inside it? Both are valid. Registering a specific handler per event can be clearer when events are very different. Registering one broader handler for a family of events and switching on `event_id` is also common, especially in small embedded modules, because it keeps the number of functions down and makes it easy to see one event family in one place.

## 6. The Core ESP-IDF Concepts

Now we move from event systems in general to ESP-IDF specifically.

The most important pieces are:

- event base
- event id
- event data
- event loop
- event handler

### Event Base

An event base is like a module family name.

Examples:

- `WIFI_EVENT`
- `IP_EVENT`

Think of it as:

"Which subsystem does this event belong to?"

### Event ID

The event ID identifies the exact event inside that family.

Examples:

- `WIFI_EVENT_STA_START`
- `WIFI_EVENT_STA_DISCONNECTED`
- `IP_EVENT_STA_GOT_IP`

So together:

- base = family
- id = specific member inside that family

That pair is basically the event's identity.

### Event Data

Some events carry extra data.

Examples:

- disconnect reason
- assigned IP address

That payload arrives through `event_data`.

This is one of the most important places to slow down, because `void *event_data` looks both powerful and dangerous. And that is exactly what it is. The type is erased at the generic API boundary, so you must know what payload type belongs to the event you are handling. That is why event identity matters so much: the producer defines what payload type belongs to a given event, and the listener must cast `event_data` to the correct type for that event.

Ownership and lifetime matter just as much as typing here. For normal ESP-IDF event posting, the right mental model is that the event system owns a copy of the posted payload long enough to dispatch it safely. When a handler runs, `event_data` points to event payload memory that is valid for that handler call. But you should not assume that pointer remains valid forever after the handler returns. So the safe rule is: use the payload during the callback, and if you need it later, copy what you need into your own storage.

So yes, `void *` is dangerous if used carelessly. But it is common in C event systems because it lets one generic API carry many payload types with low overhead.

### Event Loop

The event loop is the dispatcher.

It waits for posted events and delivers them to matching handlers.

### Event Handler

This is your callback function.

It runs when the event loop dispatches a matching event.

## 7. The Important ESP-IDF Types

These come from `components/esp_event/include/esp_event.h`.

The core callback type is conceptually:

```c
typedef void (*esp_event_handler_t)(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
);
```

Interpretation:

- `arg`: your own user pointer
- `event_base`: which family
- `event_id`: which exact event
- `event_data`: optional payload

The default event loop is created with:

```c
esp_err_t esp_event_loop_create_default(void);
```

And handlers are commonly registered with:

```c
esp_err_t esp_event_handler_instance_register(
    esp_event_base_t event_base,
    int32_t event_id,
    esp_event_handler_t event_handler,
    void *event_handler_arg,
    esp_event_handler_instance_t *context
);
```

At first, `event_id` can look strangely arbitrary, especially if your instinct was that "module + type of event" should already be one thing. What ESP-IDF is really doing is splitting that idea into two parts:

- base = module or family
- id = specific event inside that family

So this is not random complexity. It is a scoped naming system. The collision story is that you do not try to make one giant globally unique number for every event in the whole firmware. Uniqueness is scoped by base. Two different modules can both have an event whose numeric id is `0`, as long as they have different event bases. That is exactly why the base exists. The pair is what matters, not the id alone.

For your own event loops there is also an explicit version:

```c
esp_event_handler_instance_register_with(...)
```

but for most first projects the default loop is enough.

## 8. How The Default Event Loop Actually Works

When you call:

```c
esp_event_loop_create_default();
```

ESP-IDF creates the default event loop object and also creates a FreeRTOS task to run it.

In your installed ESP-IDF 6.0 source:

- `components/esp_event/default_event_loop.c`
- `components/esp_system/include/esp_task.h`

you can see that the default loop task is created with:

- task name: `sys_evt`
- priority: `ESP_TASKD_EVENT_PRIO`

And in your setup:

- `configMAX_PRIORITIES = 25`
- `ESP_TASKD_EVENT_PRIO = configMAX_PRIORITIES - 5`

So the default event task priority is `20`.

That gives a better mental model:

- the event loop lives as data in RAM
- the event loop runs inside the `sys_evt` task
- that task mostly sleeps waiting for queued events
- when an event arrives, it wakes and dispatches handlers

So no, the loop is not some magical invisible system.

It is a normal runtime object plus a FreeRTOS task.

## 9. How Events Are Stored Internally

This is worth understanding because it explains the design tradeoffs.

In `components/esp_event/esp_event.c`, ESP-IDF uses:

- a FreeRTOS queue for posted events
- linked lists for handler registration structures

That means:

- queued events are in a fixed-size queue
- registered handlers are looked up through linked structures

This is a very important distinction.

The system is not:

"one giant linked list of pending events"

It is:

- fixed queue of pending events
- linked-list registry of who listens to what

Why this design?

Because a fixed queue gives predictable buffering for events.

And linked lists give flexibility for:

- dynamic handler registration
- many event families
- wildcard handlers
- custom loops

The downside is that handler lookup is not the fastest possible.

ESP-IDF even notes in source comments that event lookup is `O(n)`.

Why is that acceptable?

Because the event system is intended for control events, not extreme throughput.

That is a deliberate design decision:

favor flexibility and decoupling over maximum lookup speed.

## 10. The Smallest Wi-Fi Example

This is the first useful real example.

```c
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Connected and got an IP address
    }
}

void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t wifi_handler;
    esp_event_handler_instance_t ip_handler;

    esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_event_handler,
        NULL,
        &wifi_handler);

    esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        wifi_event_handler,
        NULL,
        &ip_handler);

    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "MyWifi",
            .password = "MyPassword",
        },
    };

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}
```

Read this in order:

1. create the loop
2. register listeners
3. start Wi-Fi
4. Wi-Fi posts events
5. handler reacts

That order matters.

## 11. The Important Wi-Fi Event Names

From `components/esp_wifi/include/esp_wifi_types_generic.h`:

- `WIFI_EVENT_STA_START`
- `WIFI_EVENT_STA_DISCONNECTED`

From `components/esp_netif/include/esp_netif_types.h`:

- `IP_EVENT_STA_GOT_IP`

These are enough for your first real station-mode app.

Their meaning:

`WIFI_EVENT_STA_START`

The station subsystem started. A common first action here is:

```c
esp_wifi_connect();
```

`WIFI_EVENT_STA_DISCONNECTED`

The connection was lost or failed. You might:

- retry
- count attempts
- report failure

`IP_EVENT_STA_GOT_IP`

The ESP32 has fully joined the network and received an IPv4 address.

This is the moment that most app logic should treat as "connected."

That distinction matters:

Wi-Fi started is not the same thing as connected.
Connected is not the same thing as got IP.

## 12. Event Data Payloads

Some events carry useful extra data.

### Got IP

For `IP_EVENT_STA_GOT_IP`, the `event_data` type is `ip_event_got_ip_t`.

See `components/esp_netif/include/esp_netif_types.h`.

Conceptually:

```c
typedef struct {
    esp_ip4_addr_t ip;
    esp_ip4_addr_t netmask;
    esp_ip4_addr_t gw;
    bool ip_changed;
} ip_event_got_ip_t;
```

In a handler:

```c
if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    // event->ip contains the assigned IPv4 address
}
```

### Disconnected

For `WIFI_EVENT_STA_DISCONNECTED`, the payload type is `wifi_event_sta_disconnected_t`.

See `components/esp_wifi/include/esp_wifi_types_generic.h`.

That contains things like:

- SSID
- BSSID
- reason code

In a handler:

```c
if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    wifi_event_sta_disconnected_t *event =
        (wifi_event_sta_disconnected_t *)event_data;

    // event->reason tells you why disconnect happened
}
```

This is a big embedded-systems lesson:

`void *event_data` is only meaningful if you know what type belongs to that event.

So event identity and payload type are linked by convention and documentation.

## 13. Why ESP-IDF Uses Base + ID Instead Of One String Name

A string name like `"wifi_sta_disconnected"` would be easy to understand, so why not use that?

Because base + id gives some practical advantages:

- clearer module separation
- lower runtime overhead than strings
- easier wildcard matching by family
- less memory churn

The event base says:

"which subsystem family?"

The event ID says:

"which member in that family?"

That is a good fit for embedded code where static identities are common.

## 14. How Registration Actually Feels

When you register a handler, you are saying:

"If an event with this identity shows up on this loop, call this function."

Examples:

Register for exactly one event:

```c
esp_event_handler_instance_register(
    IP_EVENT,
    IP_EVENT_STA_GOT_IP,
    wifi_event_handler,
    NULL,
    &ip_handler);
```

Register for every Wi-Fi event:

```c
esp_event_handler_instance_register(
    WIFI_EVENT,
    ESP_EVENT_ANY_ID,
    wifi_event_handler,
    NULL,
    &wifi_handler);
```

That second one is why your handler often needs an `if` or `switch`.

Because you told ESP-IDF:

"Send me all Wi-Fi events."

So now your callback must inspect `event_id` and decide what to do.

## 15. One Handler Versus Many Handlers

This is a design choice you will make often.

### One big handler

Common pattern:

```c
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ...
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ...
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ...
    }
}
```

Advantages:

- easy to see the whole connection flow
- fewer functions
- nice for small modules

Disadvantages:

- can become crowded
- mixes unrelated cases if it grows

### Separate handlers

You can also do:

```c
static void on_wifi_start(...);
static void on_wifi_disconnect(...);
static void on_got_ip(...);
```

Advantages:

- clearer separation
- easier to test mentally

Disadvantages:

- more registration code
- event flow is spread across more places

There is no single right answer.

For early Wi-Fi learning, one handler is usually easier.

## 16. Creating Your Own Events

Now we move from consuming ESP-IDF events to producing your own.

Suppose you build a module called `sensor_manager`.

You want other code to know when:

- sensor started
- measurement finished
- measurement failed

### Step 1: define an event base

In your header:

```c
#pragma once

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SENSOR_EVENT);

typedef enum {
    SENSOR_EVENT_STARTED,
    SENSOR_EVENT_MEASUREMENT_READY,
    SENSOR_EVENT_ERROR,
} sensor_event_id_t;
```

That says:

- this module has one event family called `SENSOR_EVENT`
- here are the IDs in that family

At this point it is natural to ask whether `ESP_EVENT_DECLARE_BASE` really has to be in the header file. Strictly speaking, no, not in the language-lawyer sense. But it is usually the right place for it, because `ESP_EVENT_DECLARE_BASE(SENSOR_EVENT);` is a declaration. It tells other translation units that there exists an event base named `SENSOR_EVENT` somewhere. That is exactly what headers are for in C: shared declarations that many `.c` files may need.

So the normal pattern is:

In the header:

```c
ESP_EVENT_DECLARE_BASE(SENSOR_EVENT);
```

In exactly one `.c` file:

```c
ESP_EVENT_DEFINE_BASE(SENSOR_EVENT);
```

Why not put `DEFINE` in the header? Because then every `.c` file including that header would create the real definition and you would get multiple-definition linker errors. Why not put `DECLARE` only in one `.c` file? Because then other files that want to post or register `SENSOR_EVENT` would not have the shared declaration in the normal public place. So the clean C rule is:

- `DECLARE` in the header
- `DEFINE` once in one `.c` file

### Step 2: define the base in exactly one C file

In one `.c` file:

```c
#include "sensor_events.h"

ESP_EVENT_DEFINE_BASE(SENSOR_EVENT);
```

That gives the event base a real definition.

Important rule:

- declare in headers
- define once in one `.c` file

That is a normal C linkage pattern.

### Step 3: define payload structs if needed

If an event carries data:

```c
typedef struct {
    int value;
    int timestamp_ms;
} sensor_measurement_ready_t;
```

### Step 4: post an event

When your module wants to announce something:

```c
sensor_measurement_ready_t evt = {
    .value = 42,
    .timestamp_ms = 1000,
};

esp_event_post(
    SENSOR_EVENT,
    SENSOR_EVENT_MEASUREMENT_READY,
    &evt,
    sizeof(evt),
    portMAX_DELAY);
```

That is the producer side.

### Step 5: listen for it somewhere else

```c
static void sensor_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == SENSOR_EVENT &&
        event_id == SENSOR_EVENT_MEASUREMENT_READY) {

        sensor_measurement_ready_t *evt =
            (sensor_measurement_ready_t *)event_data;

        // use evt->value
    }
}
```

And register it:

```c
esp_event_handler_instance_register(
    SENSOR_EVENT,
    SENSOR_EVENT_MEASUREMENT_READY,
    sensor_event_handler,
    NULL,
    &handler_instance);
```

That is the full custom-event flow.

## 17. Full Custom Event Example

Header:

```c
// sensor_events.h
#pragma once

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SENSOR_EVENT);

typedef enum {
    SENSOR_EVENT_STARTED,
    SENSOR_EVENT_MEASUREMENT_READY,
    SENSOR_EVENT_ERROR,
} sensor_event_id_t;

typedef struct {
    int value;
    int timestamp_ms;
} sensor_measurement_ready_t;
```

Definition:

```c
// sensor_events.c
#include "sensor_events.h"

ESP_EVENT_DEFINE_BASE(SENSOR_EVENT);
```

Producer:

```c
// sensor_manager.c
#include "sensor_events.h"

static void sensor_publish_measurement(int value, int timestamp_ms)
{
    sensor_measurement_ready_t evt = {
        .value = value,
        .timestamp_ms = timestamp_ms,
    };

    esp_event_post(
        SENSOR_EVENT,
        SENSOR_EVENT_MEASUREMENT_READY,
        &evt,
        sizeof(evt),
        portMAX_DELAY);
}
```

Listener:

```c
// app_events.c
#include "sensor_events.h"

static void sensor_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == SENSOR_EVENT &&
        event_id == SENSOR_EVENT_MEASUREMENT_READY) {

        sensor_measurement_ready_t *evt =
            (sensor_measurement_ready_t *)event_data;

        // react to new sensor data
    }
}
```

Registration:

```c
esp_event_handler_instance_t sensor_handler;

esp_event_handler_instance_register(
    SENSOR_EVENT,
    SENSOR_EVENT_MEASUREMENT_READY,
    sensor_event_handler,
    NULL,
    &sensor_handler);
```

That is the complete pattern.

## 18. Do You Need Your Own Events?

Very important design question.

Just because ESP-IDF gives you an event system does not mean every module should use it.

| Situation | Better Fit |
|---|---|
| Your module reports to exactly one caller | Return values, direct call, or a simple callback |
| The relationship is one-to-one and simple | Direct call or function pointer callback |
| You are moving data between tasks | FreeRTOS queue |
| Multiple parts of the app may care | `esp_event` becomes attractive |
| The producer should not know its listeners | `esp_event` is a good fit |
| The thing happens asynchronously | `esp_event` is often a good fit |
| The event is mainly a state/lifecycle notification | `esp_event` is a strong fit |
| The data rate is high | Avoid `esp_event` as the main transport |
| A simple callback would be clearer | Prefer the simpler callback |

This is one of the main signs of design maturity:

not using a flexible tool everywhere just because it exists.

## 19. Alternatives To The Event System

To understand the tradeoffs, compare `esp_event` to alternatives.

| Mechanism | Best For | Strengths | Weaknesses |
|---|---|---|---|
| Direct function call | Simple, tight, synchronous relationships | Very simple, fast, obvious control flow | Tight coupling, producer must know consumer |
| Function pointer callback | One producer, one listener, custom asynchronous operations | Low overhead, simple, direct | Less flexible than publish/subscribe, producer still knows callback registration |
| FreeRTOS queue | Passing data between tasks, work pipelines | Explicit task-to-task communication, predictable buffering | More task-oriented than pub/sub, less natural when many listeners care |
| Event groups | Bit-style synchronization, wait-for-flags logic | Efficient synchronization primitive | Poor fit for rich payloads, poor fit for named event families |
| ESP-IDF `esp_event` | Decoupled notifications, subsystem lifecycle signals, many listeners | Flexible, consistent across IDF subsystems, good for control events | More abstraction, runtime lookup overhead, more moving parts to understand |

## 20. Source Files To Read

If you want to deepen this from source:

- `components/esp_event/include/esp_event.h`
- `components/esp_event/esp_event.c`
- `components/esp_event/default_event_loop.c`
- `components/esp_system/include/esp_task.h`
- `components/esp_wifi/include/esp_wifi_types_generic.h`
- `components/esp_netif/include/esp_netif_types.h`

Those are the files that connect the concepts in this document to the actual implementation.
