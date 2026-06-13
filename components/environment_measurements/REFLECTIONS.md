# Environment Measurements Reflections

## Design

The module separates physical acquisition, polling, storage, and the public C
API. This allows the BME280 and simulated producers to use the same manager and
store while keeping sensor-specific behavior private.

A single polling task reads producers sequentially. This avoids one task per
sensor and makes publication ordering and failure handling easier to reason
about. The tradeoff is that a slow producer delays all later producers.

The store publishes each producer batch atomically. Consumers therefore cannot
observe a new temperature together with humidity or pressure from an older
BME280 acquisition.

## Failure Handling

Sensor initialization failure does not prevent module startup. Failed channels
are invalidated, later polling retries initialization, and logs report only the
first failure and first recovery. This supports disconnected or temporarily
unavailable hardware without repeatedly exposing stale measurements.

## Memory And Task Stack

The environment polling task uses statically allocated task metadata and stack
storage. This makes its memory allocation deterministic, but the configured
stack size must be justified by measurement rather than assumption.

Follow `CHECK_STACK.md` to measure the task under representative worst-case
operation.

Measurement record:

| Item | Result |
|---|---|
| Initial configured stack | 4096 bytes |
| Test duration | Pending measurement |
| Workloads exercised | Pending measurement |
| Minimum observed free stack | Pending measurement |
| Peak measured stack usage | Pending measurement |
| Selected safety margin | Pending measurement |
| Final configured stack | Pending measurement |

Final calculation:

```text
peak measured usage = initial configured stack - minimum observed free stack
final configured stack = peak measured usage + safety margin
```

The temporary high-water-mark logging must be removed after the final
measurement because diagnostics add overhead and alter task behavior.

## Remaining Considerations

- The single polling task means producer read latency affects the complete
  polling cycle.
- The latest-value store deliberately does not retain measurement history.
- Stack sizing must be remeasured when task behavior, producer implementations,
  compiler settings, or logging behavior changes materially.
