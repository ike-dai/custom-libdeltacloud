#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "instance_state.h"

int add_to_transitions_list(struct transition **transitions, char *action,
			    char *to)
{
  struct transition *onetransition, *now, *last;

  onetransition = malloc(sizeof(struct transition));
  if (onetransition == NULL)
    return -1;

  onetransition->action = strdup_or_null(action);
  onetransition->to = strdup(to);
  onetransition->next = NULL;

  if (*transitions == NULL)
    /* First element in the list */
    *transitions = onetransition;
  else {
    now = *transitions;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = onetransition;
  }

  return 0;
}

void print_transitions_list(struct transition **transitions, FILE *stream)
{
  struct transition *now;

  if (stream == NULL)
    stream = stderr;

  now = *transitions;
  while (now != NULL) {
    fprintf(stream, "Action: %s\n", now->action);
    fprintf(stream, "To: %s\n", now->to);
    now = now->next;
  }
}

void free_transitions_list(struct transition **transitions)
{
  struct transition *now, *next;

  now = *transitions;
  while (now != NULL) {
    next = now->next;
    free(now->action);
    free(now->to);
    free(now);
    now = next;
  }

  *transitions = NULL;
}

int add_to_instance_states_list(struct instance_state **instance_states,
				char *name, struct transition *transitions)
{
  struct instance_state *oneinstance_state, *now, *last;

  oneinstance_state = malloc(sizeof(struct instance_state));
  if (oneinstance_state == NULL)
    return -1;

  oneinstance_state->name = strdup(name);
  if (transitions)
    oneinstance_state->transitions = transitions;
  else
    oneinstance_state->transitions = NULL;
  oneinstance_state->next = NULL;

  if (*instance_states == NULL)
    /* First element in the list */
    *instance_states = oneinstance_state;
  else {
    now = *instance_states;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = oneinstance_state;
  }

  return 0;
}

void print_instance_states_list(struct instance_state **instance_states,
				FILE *stream)
{
  struct instance_state *now;

  if (stream == NULL)
    stream = stderr;

  now = *instance_states;
  while (now != NULL) {
    fprintf(stream, "Name: %s\n", now->name);
    print_transitions_list(&now->transitions, stream);
    now = now->next;
  }
}

void free_instance_states_list(struct instance_state **instance_states)
{
  struct instance_state *now, *next;

  now = *instance_states;
  while (now != NULL) {
    next = now->next;
    free(now->name);
    free_transitions_list(&now->transitions);
    free(now);
    now = next;
  }

  *instance_states = NULL;
}
