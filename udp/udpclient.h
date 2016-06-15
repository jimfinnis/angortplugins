/**
 * \file
 * Brief description. Longer description.
 * 
 * \author $Author$
 * \date $Date$
 */


#ifndef __UDPCLIENT_H
#define __UDPCLIENT_H

bool udpSend(const char *msg);
void initClient(const char *host,int port);
#endif /* __UDPCLIENT_H */
