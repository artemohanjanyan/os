READ_TAG = "read"
WRITE_TAG = "write"
EXIT_TAG = "exit"
FORK_TAG = "fork"
EXEC_TAG = "exec"

def kernel(main, args, stdin):
    print("Running process {} with args={}, stdin={}".format(main, args, stdin))
    processes = [(main, args)]
    while processes:
        (prog, args) = processes.pop()
        (tag, sargs, cont) = prog(*args)
        if tag == READ_TAG:
            res = stdin[0]
            stdin = stdin[1:]
            processes.append((cont, [res]))
        elif tag == WRITE_TAG:
            print('STDOUT: ', sargs[0])
            processes.append((cont, []))
        elif tag == EXIT_TAG:
            print("Exit code: {}".format(prog, sargs[0]))
            pass
        elif tag == FORK_TAG:
            processes.append((cont, [1]))
            processes.append((cont, [0]))
        elif tag == EXEC_TAG:
            processes.append((sargs[0], sargs[1:]))
        else:
            print("ERROR: No such syscall")


# name = read()
# s = "hello, " + name
# write(s)
# exit(0)

def hello():
    return (READ_TAG, [], hello_1)

def hello_1(name):
    s = "hello, " + name
    return (WRITE_TAG, [s], hello_2)

def hello_2():
    return (EXIT_TAG, [0], None)

kernel(hello, [], ["Andrey"])

# flag = fork()
# if flag:
#     write("Parent")
# else:
#     write("Child")
# exit(0)

def fork():
    return (FORK_TAG, [], fork_1)

def fork_1(flag):
    if flag:
        return (WRITE_TAG, ["Parent"], fork_2)
    else:
        return (WRITE_TAG, ["Child"], fork_2)

def fork_2():
    return (EXIT_TAG, [0], None)
    
kernel(fork, [], [])

# def printer(line):
#     write(line)
#     exit(0)

def printer(line):
    return (WRITE_TAG, [line], printer_1)

def printer_1():
    return (EXIT_TAG, [0], None)

kernel(printer, ["hello"], [])

# exec(printer, ["hello2"])

def exec():
    return (EXEC_TAG, [printer, "hello2"], None)
    
kernel(exec, [], [])

# s = "1"
# count = 0
# while s != "exit":
#     count += 1
#     s = read()
#     write(s)
# exit(count)

def cycle():
    env = {}
    env["count"] = 0
    env["s"] = "1"
    def cycle_while():
        if env["s"] == "exit":
            return (EXIT_TAG, [env["count"]], None)
        else:
            env["count"] += 1
            def cycle_while_1(s):
                env["s"] = s
                return (WRITE_TAG, [env["s"]], cycle_while)
            return (READ_TAG, [], cycle_while_1)
    return cycle_while()

kernel(cycle, [], ["one", "two", "three", "exit", "four", "five"])
