#ifndef BASE_LINKED_LISTS_H

#define IsNil(v) (v == nil)

#define SLLStackPushN(h, n, next) (IsNil(h) ? (h = n) : (n->next = h, h = n))
#define SLLStackPush(h, n) SLLStackPushN(h, n, next)

#define SSLQueuePushN(h, t, n, next) \
   (IsNil(h) ? (h = n) : (IsNil(t) ? (h->next = t = n) : (t->next = n, t = n)))
#define SLLQueuePush(h, t, n) SSLQueuePushN(h, t, n, next)

#define BASE_LINKED_LISTS_H
#endif
