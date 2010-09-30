
#ifndef SESSION_H__
#define SESSION_H__

struct _client;

enum ProxyState {
  ProxyState_PAUSED,
  ProxyState_SENDING,
  ProxyState_STOPPED
};

typedef struct _session {
  enum ProxyState state;

  int client_count;
  struct _client* clients;

  // All client connections in this session are forwarded to this address:port
  char* downstream_address;
  int   downstream_port;
} Session;

void session_add_client (Session *session, struct _client *client);
void session_remove_client (Session *session, struct _client *client);

#endif /* SESSION_H__ */
