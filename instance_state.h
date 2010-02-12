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

int add_to_transition_list(struct transition **transitions, char *action,
			    char *to);
void print_transition_list(struct transition **transitions, FILE *stream);
void free_transition_list(struct transition **transitions);

int add_to_instance_state_list(struct instance_state **instance_states,
				char *name, struct transition *transitions);
void print_instance_state_list(struct instance_state **instance_states,
				FILE *stream);
void free_instance_state_list(struct instance_state **instance_states);

#endif
