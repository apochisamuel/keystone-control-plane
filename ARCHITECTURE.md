# Architecture

Aegis processes a capture in stages: byte decoding, frame parsing, field
validation, session mutation, policy application, routing, dispatch planning,
diagnostics, and optional persistence or recovery. Session identity is stable
across compatible profile changes while profile generations are tracked for
observability. Policy and routing operations are applied through explicit
stage/commit/rollback transitions.

The engine keeps several indexed views of the same state: sessions, routes,
policy records, audit events, topology graphs, replay windows, and recovery
entries. This arrangement supports realistic capture inspection workflows where
operations arrive interleaved across multiple sessions.
