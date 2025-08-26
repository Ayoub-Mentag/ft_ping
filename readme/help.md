# Ping Command Usage

Send ICMP ECHO_REQUEST packets to network hosts.

---

## Options controlling ICMP request types

- `--address`  
  Send ICMP_ADDRESS packets (**root only**)

- `--echo`  
  Send ICMP_ECHO packets (**default**)

- `--mask`  
  Same as `--address`

- `--timestamp`  
  Send ICMP_TIMESTAMP packets

- `-t, --type=TYPE`  
  Send TYPE packets

---

## Options valid for all request types

- `-c, --count=NUMBER`  
  Stop after sending NUMBER packets

- `-d, --debug`  
  Set the SO_DEBUG option

- `-i, --interval=NUMBER`  
  Wait NUMBER seconds between sending each packet

- `-n, --numeric`  
  Do not resolve host addresses

- `-r, --ignore-routing`  
  Send directly to a host on an attached network

- `--ttl=N`  
  Specify N as time-to-live

- `-T, --tos=NUM`  
  Set type of service (TOS) to NUM

- `-v, --verbose`  
  Verbose output

- `-w, --timeout=N`  
  Stop after N seconds

- `-W, --linger=N`  
  Number of seconds to wait for response

---

## Options valid for `--echo` requests

- `-f, --flood`  
  Flood ping (**root only**)

- `--ip-timestamp=FLAG`  
  IP timestamp of type FLAG, which is one of `tsonly` and `tsaddr`

- `-l, --preload=NUMBER`  
  Send NUMBER packets as fast as possible before falling into normal mode of behavior (**root only**)

- `-p, --pattern=PATTERN`  
  Fill ICMP packet with given pattern (hex)

- `-q, --quiet`  
  Quiet output

- `-R, --route`  
  Record route

- `-s, --size=NUMBER`  
  Send NUMBER data octets

---

## General Options

- `-?, --help`  
  Show help list

- `--usage`  
  Give a short usage message

- `-V, --version`  
  Print program version

---

### Notes

- Mandatory or optional arguments to long options are also mandatory or optional for corresponding short options.  
- Options marked with **(root only)** are available only to the superuser.  

---

📌 Report bugs to: [bug-inetutils@gnu.org](mailto:bug-inetutils@gnu.org)
