client <- function(num) {
    library(rzmq)
    context <- init.context()
    client <- init.socket(context, "ZMQ_REQ")
    connect.socket(client, "tcp://localhost:5559")
    send.socket(client, num)
    cat(sprintf("client %d receiving:%d\n", num, receive.socket(client)))
}
client1 <- parallel::mcparallel(client(1))
client2 <- parallel::mcparallel(client(2))

# SERVER

library(rzmq)
context <- init.context()
frontend <- init.socket(context, "ZMQ_ROUTER")
backend <- init.socket(context, "ZMQ_DEALER")
bind.socket(frontend, "tcp://*:5559")
bind.socket(backend, "inproc://workers")

worker1 <- init.socket(context, "ZMQ_REP")
connect.socket(worker1, "inproc://workers")
input1 <- receive.multipart(frontend)
send.multipart(backend, input1)

x <- receive.socket(worker1)
cat(sprintf("worker 1  receiving:%d\n", x))

worker2 <- init.socket(context, "ZMQ_REP")
connect.socket(worker2, "inproc://workers")
input2 <- receive.multipart(frontend)
send.multipart(backend, input2)

y <- receive.socket(worker2)

send.socket(worker1, x)
output1 <- receive.multipart(backend)
send.multipart(frontend, output1)

send.socket(worker2, y)
output2 <- receive.multipart(backend)
send.multipart(frontend, output2)
parallel::mccollect(list(client1, client2), wait)
