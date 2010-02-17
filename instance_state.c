#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "instance_state.h"

static void free_transition(struct transition *transition)
{
  MY_FREE(transition->action);
  MY_FREE(transition->to);
}

int add_to_transition_list(struct transition **transitions, const char *action,
			   const char *to)
{
  struct transition *onetransition, *curr, *last;

  onetransition = malloc(sizeof(struct transition));
  if (onetransition == NULL)
    return -1;

  memset(onetransition, 0, sizeof(struct transition));

  if (strdup_or_null(&onetransition->action, action) < 0)
    goto error;
  if (strdup_or_null(&onetransition->to, to) < 0)
    goto error;
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

 error:
  free_transition(onetransition);
  MY_FREE(onetransition);
  return -1;
}

static int copy_transition_list(struct transition **dst,
				 struct transition **src)
{
  struct transition *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    if (add_to_transition_list(dst, curr->action, curr->to) < 0)
      goto error;
    curr = curr->next;
  }

  return 0;

 error:
  free_transition_list(dst);
  return -1;
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
    free_transition(curr);
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

  memset(oneinstance_state, 0, sizeof(struct instance_state));

  if (strdup_or_null(&oneinstance_state->name, name) < 0)
    goto error;
  if (copy_transition_list(&oneinstance_state->transitions, &transitions) < 0)
    goto error;
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

 error:
  free_instance_state(oneinstance_state);
  MY_FREE(oneinstance_state);
  return -1;
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

int copy_instance_state(struct instance_state *dst, struct instance_state *src)
{
  memset(dst, 0, sizeof(struct instance_state));

  if (strdup_or_null(&dst->name, src->name) < 0)
    goto error;
  if (copy_transition_list(&dst->transitions, &src->transitions) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  free_instance_state(dst);
  return -1;
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
