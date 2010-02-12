#ifndef INSTANCE_STATES_H
#define INSTANCE_STATES_H

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

int add_to_transitions_list(struct transition **transitions, char *action,
			    char *to);
void print_transitions_list(struct transition **transitions, FILE *stream);
void free_transitions_list(struct transition **transitions);

int add_to_instance_states_list(struct instance_state **instance_states,
				char *name, struct transition *transitions);
void print_instance_states_list(struct instance_state **instance_states,
				FILE *stream);
void free_instance_states_list(struct instance_state **instance_states);

#endif
