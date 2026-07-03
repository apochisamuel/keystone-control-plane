# Capture Format

A capture is a concatenation of frames. Each frame has a four-byte `AGF1`
marker, a one-byte operation, one-byte flags, a two-byte big-endian payload
length, and a four-byte big-endian session identifier. The payload is a series
of fields encoded as one-byte tag, two-byte big-endian field length, and value
bytes.

Unknown fields are retained by extension-aware paths where possible. Malformed
frames are rejected without terminating the overall capture pass.
