#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "instance_state.h"

int add_to_transition_list(struct transition **transitions, const char *action,
			   const char *to)
{
  struct transition *onetransition, *curr, *last;

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
    curr = *transitions;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = onetransition;
  }

  return 0;
}

static void copy_transition_list(struct transition **dst,
				 struct transition **src)
{
  struct transition *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    add_to_transition_list(dst, curr->action, curr->to);
    curr = curr->next;
  }
}

void print_transition_list(struct transition **transitions, FILE *stream)
{
  struct transition *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *transitions;
  while (curr != NULL) {
    fprintf(stream, "Action: %s\n", curr->action);
    fprintf(stream, "To: %s\n", curr->to);
    curr = curr->next;
  }
}

void free_transition_list(struct transition **transitions)
{
  struct transition *curr, *next;

  curr = *transitions;
  while (curr != NULL) {
    next = curr->next;
    MY_FREE(curr->action);
    MY_FREE(curr->to);
    MY_FREE(curr);
    curr = next;
  }

  *transitions = NULL;
}

int add_to_instance_state_list(struct instance_state **instance_states,
			       const char *name,
			       struct transition *transitions)
{
  struct instance_state *oneinstance_state, *curr, *last;

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
    curr = *instance_states;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneinstance_state;
  }

  return 0;
}

struct instance_state *find_by_name_in_instance_state_list(struct instance_state **instance_states,
							   const char *name)
{
  struct instance_state *curr;

  curr = *instance_states;
  while (curr != NULL) {
    if (STREQ(curr->name, name))
      return curr;
    curr = curr->next;
  }

  return NULL;
}

void copy_instance_state(struct instance_state *dst, struct instance_state *src)
{
  dst->name = strdup_or_null(src->name);
  copy_transition_list(&dst->transitions, &src->transitions);
  dst->next = NULL;
}

void print_instance_state(struct instance_state *instance_state,
			  FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Name: %s\n", instance_state->name);
  print_transition_list(&instance_state->transitions, stream);
}

void print_instance_state_list(struct instance_state **instance_states,
			       FILE *stream)
{
  struct instance_state *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *instance_states;
  while (curr != NULL) {
    print_instance_state(curr, NULL);
    curr = curr->next;
  }
}

void free_instance_state(struct instance_state *instance_state)
{
  MY_FREE(instance_state->name);
  free_transition_list(&instance_state->transitions);
}

void free_instance_state_list(struct instance_state **instance_states)
{
  struct instance_state *curr, *next;

  curr = *instance_states;
  while (curr != NULL) {
    next = curr->next;
    free_instance_state(curr);
    MY_FREE(curr);
    curr = next;
  }

  *instance_states = NULL;
}
