# Example: Print "Hello, world!" followed by a newline to stdout using sys_write, then exit.
# Syscall details:
#  sys_write: rax=1, rdi=stdout, rsi=buffer, rdx=length.
#  sys_exit:  rax=60, rdi=exit status.
#
# The entry point is at the beginning of the code section.

# sys_write(stdout, msg, 14)
move rax, 1       # sys_write
move rdi, 1       # stdout file descriptor
move rsi, msg     # address of message (resolved via symbol)
move rdx, 14      # message length
call

# sys_exit(0)
move rax, 60      # sys_exit
move rdi, 0       # exit status 0
call

# Data section: define label 'msg' with the message.
data msg "Hello, world!\n"