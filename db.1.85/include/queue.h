#ifndef __CIRCLE_QUEUE__
#define __CIRCLE_QUEUE__

#define CIRCLEQ_HEAD(name, type)                                        \
struct name {                                                           \
		struct type *cqh_first;         /* first element */             \
		struct type *cqh_last;          /* last element */              \
}

#define CIRCLEQ_ENTRY(type)                                             \
struct {                                                                \
		struct type *cqe_next;          /* next element */              \
		struct type *cqe_prev;          /* previous element */          \
}

#define CIRCLEQ_INIT(head) do {                                         \
		(head)->cqh_first = (void *)(head);                             \
		(head)->cqh_last = (void *)(head);                              \
} while (/*CONSTCOND*/0)

#define CIRCLEQ_INSERT_HEAD(head, elm, field) do {                      \
		(elm)->field.cqe_next = (head)->cqh_first;                      \
		(elm)->field.cqe_prev = (void *)(head);                         \
		if ((head)->cqh_last == (void *)(head))                         \
				(head)->cqh_last = (elm);                               \
		else                                                            \
				(head)->cqh_first->field.cqe_prev = (elm);              \
		(head)->cqh_first = (elm);                                      \
} while (/*CONSTCOND*/0)

#define CIRCLEQ_INSERT_TAIL(head, elm, field) do {                      \
		(elm)->field.cqe_next = (void *)(head);                         \
		(elm)->field.cqe_prev = (head)->cqh_last;                       \
		if ((head)->cqh_first == (void *)(head))                        \
				(head)->cqh_first = (elm);                              \
		else                                                            \
				(head)->cqh_last->field.cqe_next = (elm);               \
		(head)->cqh_last = (elm);                                       \
} while (/*CONSTCOND*/0)

#define CIRCLEQ_REMOVE(head, elm, field) do {                           \
		if ((elm)->field.cqe_next == (void *)(head))                    \
				(head)->cqh_last = (elm)->field.cqe_prev;               \
		else                                                            \
				(elm)->field.cqe_next->field.cqe_prev =                 \
					(elm)->field.cqe_prev;                              \
		if ((elm)->field.cqe_prev == (void *)(head))                    \
				(head)->cqh_first = (elm)->field.cqe_next;              \
		else															\
			(elm)->field.cqe_prev->field.cqe_next =						\
					(elm)->field.cqe_next;                              \
} while (/*CONSTCOND*/0)

#endif

