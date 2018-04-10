/* Yuen Han Chan */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rtp.h"

/* GIVEN Function:
 * Handles creating the client's socket and determining the correct
 * information to communicate to the remote server
 */
CONN_INFO* setup_socket(char* ip, char* port){
    struct addrinfo *connections, *conn = NULL;
    struct addrinfo info;
    memset(&info, 0, sizeof(struct addrinfo));
    int sock = 0;
    
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_DGRAM;
    info.ai_protocol = IPPROTO_UDP;
    getaddrinfo(ip, port, &info, &connections);
    
    /*for loop to determine corr addr info*/
    for(conn = connections; conn != NULL; conn = conn->ai_next){
        sock = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
        if(sock <0){
            if(DEBUG)
                perror("Failed to create socket\n");
            continue;
        }
        if(DEBUG)
            printf("Created a socket to use.\n");
        break;
    }
    if(conn == NULL){
        perror("Failed to find and bind a socket\n");
        return NULL;
    }
    CONN_INFO* conn_info = malloc(sizeof(CONN_INFO));
    conn_info->socket = sock;
    conn_info->remote_addr = conn->ai_addr;
    conn_info->addrlen = conn->ai_addrlen;
    return conn_info;
}

void shutdown_socket(CONN_INFO *connection){
    if(connection)
        close(connection->socket);
}

/*
 * ===========================================================================
 *
 *			STUDENT CODE STARTS HERE. PLEASE COMPLETE ALL FIXMES
 *
 * ===========================================================================
 */


/*
 *  Returns a number computed based on the data in the buffer.
 */
static int checksum(char *buffer, int length){
    int sum = 0;
    for(int i = 0; i<length; i++){
        sum+=buffer[i];
    }
    return sum;
    
    
    /*  ----  FIXED  ----
     *
     *  The goal is to return a number that is determined by the contents
     *  of the buffer passed in as a parameter.  There a multitude of ways
     *  to implement this function.  For simplicity, simply sum the ascii
     *  values of all the characters in the buffer, and return the total.
     */
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it
 *  contains the length of the array created.
 */
static PACKET* packetize(char *buffer, int length, int *count){
    /*  ----  FIXED  ----
     *  The goal is to turn the buffer into an array of packets.
     *  You should allocate the space for an array of packets and
     *  return a pointer to the first element in that array.  Each
     *  packet's type should be set to DATA except the last, as it
     *  should be LAST_DATA type. The integer pointed to by 'count'
     *  should be updated to indicate the number of packets in the
     *  array.
     */
    
    if(length%MAX_PAYLOAD_LENGTH==0){
        *count = length/MAX_PAYLOAD_LENGTH;
    }else{
        *count = length/MAX_PAYLOAD_LENGTH+1;
    }
    PACKET* packetArray = malloc(sizeof(PACKET)* (*count));
    PACKET* temp = packetArray;
    for(int i=0; i<length; i++){
        int currentPackage = i/MAX_PAYLOAD_LENGTH;
        int index = i%MAX_PAYLOAD_LENGTH;
        temp = packetArray+currentPackage;
        temp->payload[index] = buffer[i];
        if(i==length-1){
            temp->type=LAST_DATA;
            temp->payload_length=index+1;
        }
        else{
            temp->payload_length = MAX_PAYLOAD_LENGTH;
            temp->type=DATA;
        }
        temp->checksum = checksum(temp->payload, temp->payload_length);
    }
    return packetArray;
}

/*
 * Send a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
int rtp_send_message(CONN_INFO *connection, MESSAGE*msg){
    /* ---- FIXED ----
     * The goal of this function is to turn the message buffer
     * into packets and then, using stop-n-wait RTP protocol,
     * send the packets and re-send if the response is a NACK.
     * If the response is an ACK, then you may send the next one
     */
    
    int count = 0;
    PACKET* messagePackage = packetize(msg->buffer, msg->length, &count);
    int sendFile = 0;
    while(sendFile<count){
        sendto(connection->socket,&messagePackage[sendFile],sizeof(PACKET),0,connection->remote_addr,connection->addrlen);
        PACKET* received = malloc(sizeof(PACKET));
        recvfrom(connection->socket, received, sizeof(PACKET), 0, connection->remote_addr, &connection->addrlen);
        if(received->type==ACK){
            sendFile++;
        }
    }
    return 1;
}

/*
 * Receive a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
MESSAGE* rtp_receive_message(CONN_INFO *connection){
    MESSAGE* message = malloc(sizeof(MESSAGE));
    int lastDataNotReceived = 1;
    while(lastDataNotReceived){
        PACKET* store = malloc(sizeof(PACKET));
        recvfrom(connection->socket, store, sizeof(PACKET), 0, connection->remote_addr, &connection->addrlen);
        int packetSum = checksum(store->payload, store->payload_length);
        if(packetSum==store->checksum){
            PACKET* send = malloc(sizeof(PACKET));
            send->type = ACK;
            sendto(connection->socket, send, sizeof(PACKET), 0, connection->remote_addr, connection->addrlen);
            int totalLength = message->length + store->payload_length;
            char* buffer = malloc(sizeof(char)*totalLength);
            
            for(int i = 0; i<message->length; i++){
                buffer[i]=message->buffer[i];
            }
            for(int i = message->length; i<totalLength; i++){
                int payloadCurrent = i-(message->length);
                buffer[i]=store->payload[payloadCurrent];
            }
            message->buffer = buffer;
            message->length = totalLength;
        }else{
            PACKET* send = malloc(sizeof(PACKET));
            send->type = NACK;
            sendto(connection->socket, send, sizeof(PACKET), 0, connection->remote_addr, connection->addrlen);
            continue;
        }
        if(store->type==LAST_DATA){
            lastDataNotReceived=0;
        }
    }
    return message;
    /* ---- FIXED ----
     * The goal of this function is to handle
     * receiving a message from the remote server using
     * recvfrom and the connection info given. You must
     * dynamically resize a buffer as you receive a packet
     * and only add it to the message if the data is considered
     * valid. The function should return the full message, so it
     * must continue receiving packets and sending response
     * ACK/NACK packets until a LAST_DATA packet is successfully
     * received.
     */
}
