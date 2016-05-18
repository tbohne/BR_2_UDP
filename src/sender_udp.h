#ifndef SENDER_UDP_H_
#define SENDER_UDP_H_

void prepareHeader(char *buffer, unsigned short nlength, char *name, unsigned long filelength);

void getSha1(char *buff, int bufferlength);

#endif
