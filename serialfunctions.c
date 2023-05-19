#include "notapp.h"

//serialize an unsigned int
unsigned char* serialize_ulong(unsigned char* buffer, unsigned int value){
    buffer[0] = value >> 24;
    buffer[1] = value >> 16;
    buffer[2] = value >> 8;
    buffer[3] = value;
    return buffer + 4;
}

//deserialize an unsigned int
unsigned char* deserialize_ulong(unsigned char* buffer, unsigned int *result){    
    unsigned int val = 0;
    val = val | buffer[0];
    val = val << 8;
    val = val | buffer[1];
    val = val << 8;
    val = val | buffer[2];
    val = val << 8;
    val = val | buffer[3];
    *result = val;
    return buffer + 4;
}

//serialize a string of characters
unsigned char* serialize_chr_stream(unsigned char* buffer, char *str, int str_len){
    for(int i = 0; i < str_len; i++){
        buffer[i] = str[i];
    }
    return buffer + str_len;
}

//deserailize a string of characters
unsigned char* deserialize_chr_stream(unsigned char* buffer, char *str, int str_len){
    for(int i = 0; i < str_len; i++){
        str[i] = buffer[i];
    }
    str[str_len] = '\0';
    return buffer + str_len;
}

//serialize an event struct from server based on protocal described in readme
unsigned char* serialize_event(unsigned char* buffer, struct obs_event e){
    buffer = serialize_ulong(buffer, e.sec);
    buffer = serialize_ulong(buffer, e.usec);
    
    buffer = serialize_ulong(buffer, strlen(e.host));
    buffer = serialize_chr_stream(buffer, e.host, strlen(e.host));
    buffer = serialize_ulong(buffer, strlen(e.fod));
    buffer = serialize_chr_stream(buffer, e.fod, strlen(e.fod));
    buffer = serialize_ulong(buffer, strlen(e.event));
    buffer = serialize_chr_stream(buffer, e.event, strlen(e.event));
    return buffer;
}

//deserialize an event sent from server
unsigned char* deserialize_event(unsigned char* buffer, struct u_event *e){
    unsigned int s, u;
    buffer = deserialize_ulong(buffer, &s);
    buffer = deserialize_ulong(buffer, &u);
    e->sec = s;
    e->usec = u;
    unsigned int h_len, f_len, e_len;
    buffer = deserialize_ulong(buffer, &h_len);
    char h[h_len+1];
    buffer = deserialize_chr_stream(buffer, h, h_len);
    buffer = deserialize_ulong(buffer, &f_len);
    char f[f_len+1];
    buffer = deserialize_chr_stream(buffer, f, f_len);
    buffer = deserialize_ulong(buffer, &e_len);
    char ev[e_len+1];
    buffer = deserialize_chr_stream(buffer, ev, e_len);

    strncpy(e->host, h, 64);
    strncpy(e->fod, f, 256);
    strncpy(e->event, ev, 256);
    
    return buffer;
}

