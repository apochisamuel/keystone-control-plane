# Recovery

Snapshots capture selected session descriptors, route labels, and policy
metadata. Recovery imports descriptors into a bounded in-memory ledger and
reconciles pending handover work after the new entries have been accepted.

The recovery subsystem is deterministic: it does not access clocks, sockets,
or host configuration. Recovery inputs are encoded as ordinary capture frames.
