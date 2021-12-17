#include <unistd.h>
#include <stdio.h>
#include <zmq.h>
#include <sys/wait.h>

int client(int num)
{
    void *context, *client;
    int buf[1];

    context = zmq_ctx_new();
    client = zmq_socket(context, ZMQ_REQ);
    zmq_connect(client, "tcp://localhost:5559");
    *buf = num;
    zmq_send(client, buf, 1, 0);
    *buf = 0;
    zmq_recv(client, buf, 1, 0);
    printf("client %d receiving: %d\n", num, *buf);
    zmq_close(client);
    zmq_ctx_destroy(context);
    return 0;
}

void multipart_proxy(void *from, void *to)
{
    zmq_msg_t message;

    while (1) {
        zmq_msg_init(&message);
        zmq_msg_recv(&message, from, 0);
        int more = zmq_msg_more(&message);
        zmq_msg_send(&message, to, more ? ZMQ_SNDMORE : 0);
        zmq_msg_close(&message);
        if (!more) break;
    }

}

int main(void)
{
    int status;
    if (fork() == 0) {
        client(1);
        return(0);
    }
    if (fork() == 0) {
        client(2);
        return 0;
    }
    /* SERVER */
    void *context, *frontend, *backend, *worker1, *worker2;
    int wbuf1[1], wbuf2[1];

    context = zmq_ctx_new();
    frontend = zmq_socket(context, ZMQ_ROUTER);
    backend = zmq_socket(context, ZMQ_DEALER);
    zmq_bind(frontend, "tcp://*:5559");
    zmq_bind(backend, "inproc://workers");

    worker1 = zmq_socket(context, ZMQ_REP);
    zmq_connect(worker1, "inproc://workers");
    multipart_proxy(frontend, backend);
    *wbuf1 = 0;
    zmq_recv(worker1, wbuf1, 1, 0);
    printf("worker1 receiving: %d\n", *wbuf1);

    worker2 = zmq_socket(context, ZMQ_REP);
    zmq_connect(worker2, "inproc://workers");
    multipart_proxy(frontend, backend);
    *wbuf2 = 0;
    zmq_recv(worker2, wbuf2, 1, 0);
    printf("worker2 receiving: %d\n", *wbuf2);

    zmq_send(worker1, wbuf1, 1, 0);
    multipart_proxy(backend, frontend);

    zmq_send(worker2, wbuf2, 1, 0);
    multipart_proxy(backend, frontend);

    wait(&status);
    zmq_close(frontend);
    zmq_close(backend);
    zmq_close(worker1);
    zmq_close(worker2);
    zmq_ctx_destroy(context);
    return 0;
}
