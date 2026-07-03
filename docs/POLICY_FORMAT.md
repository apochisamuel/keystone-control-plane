# Policy Format

Policy inputs are represented as fields in stage frames. Route identifiers,
profile identifiers, route labels, optional extension blocks, and policy mode
flags are compiled into an immutable active policy record at commit time.

The policy compiler canonicalizes route and extension data before the active
record is installed. Rollback removes staged and active policy artifacts for a
session while retaining an audit summary.
