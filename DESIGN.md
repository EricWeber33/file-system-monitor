
TIME REFINEMENT WAS DONE

PROTOCAL:
The first message sent by a client will be either a 1 if it is an
observer or a 0 if it is user.

PROTOCAL FOR OBSERVER TO SERVER COMMUNICATION:
the handshake is of following format:
bytes 0-3 -> (unsigned int) 1
      4-7 -> (unsigned int) length of string for item monitored n
     8-11 -> (unsigned int) length of host name for item connected m
   next n -> (string) item monitored
   next m -> (string) host name NOTE: this part is no longer used

events from observer are sent in following format
bytes 0-3 -> (unsigned int) how long buffer sent is
      4-7 -> (unsigned int) seconds
     8-11 -> (unsigned int) microseconds
     rest -> (string) string representation of inotify event

a buffer with a length 0 signals that the observer is disconnecting

PROTOCAL FOR SERVER TO USER COMMUNICATION:
the handshake if of following format:
bytes 0-3 -> (unsigned int) 0

events are sent to user in following format:
bytes 0-3 -> (unsigned int) number of structs being sent over

following bytes in the buffer represent structures and the bytes will
be labelled relative to the start of the structure
bytes 0-3 -> (unsigned int) seconds
      4-7 -> (unsigned int) microseconds
     8-11 -> (unsigned int) length of hostname h
   next h -> (string) hostname
   next 4 -> (unsigned int) length of file or directory name f
   next f -> (string) file or directory name
   next 4 -> (unsigned int) length of event text e
   next e -> (string) event text

furthermore upon recieving a message from the server the user sends a message back 1
if this message is instead 0 the user client is disconnecting.

OTHER IMPORTANT INFO:
    - gnu99 was used as the standard as sigaction and sigsetjmp and siglongjmp were used
      to handle client termination.
    - this project is implemented in a way that supports a maximum of 512 observers being connected
      at any time in total.
    - furthermore directory and file names to be monitored can be a max of 255 chars.

