#ifndef INSTANCE_STATE_H
#define INSTANCE_STATE_H

struct transition {
  char *action;
  char *to;

  struct transition *next;
};

struct instance_state {
  char *name;
  struct transition *transitions;

  struct instance_state *next;
};

int add_to_transition_list(struct transition **transitions, const char *action,
			   const char *to);
void print_transition_list(struct transition **transitions, FILE *stream);
void free_transition_list(struct transition **transitions);

int add_to_instance_state_list(struct instance_state **instance_states,
			       const char *name,
			       struct transition *transitions);
struct instance_state *find_by_name_in_instance_state_list(struct instance_state **instance_states,
							   const char *name);
void copy_instance_state(struct instance_state *dst, struct instance_state *src);
void print_instance_state(struct instance_state *instance_states,
			  FILE *stream);
void print_instance_state_list(struct instance_state **instance_states,
			       FILE *stream);
void free_instance_state(struct instance_state *instance_state);
void free_instance_state_list(struct instance_state **instance_states);

#endif
