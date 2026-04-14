#include "gcc-plugin.h"
#include "input.h"
#include "plugin-version.h"

#include "diagnostic.h"
#include "stringpool.h"
#include "tree-core.h"
#include "tree.h"

#include "attribs.h"
#include <cstring>

// Required field for plugin to work
int plugin_is_GPL_compatible;

extern void debug_tree(tree);

/**
 * @brief Copy a FIELD_DECL with a given context and isolate from chain
 *
 * @param field The FIELD_DECL node to copy
 * @param context The context for the FIELD_DECL e.g. a struct
 *
 * @return The copied field isolated from the original chain
 */
static tree copy_field(tree field, tree context) {
  tree new_field = build_decl(DECL_SOURCE_LOCATION(field), FIELD_DECL,
                              DECL_NAME(field), TREE_TYPE(field));

  DECL_FIELD_BIT_OFFSET(new_field) = DECL_FIELD_BIT_OFFSET(field);
  DECL_ALIGN_RAW(new_field) = DECL_ALIGN_RAW(field);

  SET_DECL_OFFSET_ALIGN(new_field, DECL_OFFSET_ALIGN(field));
  DECL_NOT_FLEXARRAY(new_field) = DECL_NOT_FLEXARRAY(field);
  DECL_FIELD_OFFSET(new_field) = DECL_FIELD_OFFSET(field);

  DECL_ATTRIBUTES(new_field) = DECL_ATTRIBUTES(field);

  DECL_SIZE(new_field) = DECL_SIZE(field);
  DECL_SIZE_UNIT(new_field) = DECL_SIZE_UNIT(field);

  DECL_CONTEXT(new_field) = context;
  DECL_CHAIN(new_field) = NULL_TREE;

  DECL_ARTIFICIAL(new_field) = 1;

  return new_field;
}

static void remove_attribute(tree node, const char *attribute) {
  for (tree *attr_ptr = &DECL_ATTRIBUTES(node); *attr_ptr;) {
    if (is_attribute_p(attribute, get_attribute_name(*attr_ptr))) {
      *attr_ptr = TREE_CHAIN(*attr_ptr);
    } else {
      attr_ptr = &TREE_CHAIN(*attr_ptr);
    }
  }
}

static bool contains_field(tree type, tree identifier) {
  if (type == NULL_TREE)
    return false;
  if (identifier == NULL_TREE)
    return false;
  if (TREE_CODE(type) != RECORD_TYPE)
    return false;

  const char *id = IDENTIFIER_POINTER(identifier);
  if (id == NULL)
    return false;

  for (tree field = TYPE_FIELDS(type); field; field = TREE_CHAIN(field)) {
    if (DECL_NAME(field) == NULL_TREE) {
      if (contains_field(TREE_TYPE(field), identifier)) {
        return true;
      }
    }
    if (DECL_NAME(field) != NULL_TREE &&
        strcmp(IDENTIFIER_POINTER(DECL_NAME(field)), id) == 0) {
      return true;
    }
  }

  return false;
}

static tree copy_struct_no_duplicate(tree type, tree conflicting_type) {
  tree new_type = make_node(RECORD_TYPE);
  TYPE_NAME(new_type) = NULL_TREE;

  tree fields = NULL_TREE;
  tree *tail = &fields;
  for (tree field = TYPE_FIELDS(type); field; field = DECL_CHAIN(field)) {
    if (contains_field(conflicting_type, DECL_NAME(field))) {
      continue;
    }

    tree new_field = copy_field(field, new_type);
    DECL_CHAIN(new_field) = fields;
    fields = new_field;
  }

  TYPE_FIELDS(new_type) = fields;

  return new_type;
}

static tree get_list_fields(tree type) {
  if (TREE_CODE(type) != RECORD_TYPE)
    return NULL_TREE;
  tree list = NULL_TREE;

  for (tree field = TYPE_FIELDS(type); field; field = DECL_CHAIN(field)) {
    if (TREE_CODE(field) != FIELD_DECL)
      continue;

    if (DECL_NAME(field) == NULL_TREE) {
      // warning(UNKNOWN_LOCATION, "anonymous field not implemented");
      tree anon_fields = get_list_fields(TREE_TYPE(field));
      tree *tail;
      for (tail = &anon_fields; *tail;) {
        if (TREE_CHAIN(*tail) == NULL_TREE)
          break;
        tail = &TREE_CHAIN(*tail);
      }
      TREE_CHAIN(*tail) = list;
      list = anon_fields;
    } else {
      list = tree_cons(NULL_TREE, field, list);
    }
  }
  return list;
}

static void finish_type(void *event_data, void *data) {
  tree type = (tree)event_data;

  if (!type)
    return;

  if (TREE_CODE(type) != RECORD_TYPE)
    return;

  tree *prev;
  // warning(UNKNOWN_LOCATION, "%s contains field 'b': %d",
  //         TYPE_NAME(type) ? IDENTIFIER_POINTER(TYPE_NAME(type)) :
  //         "Anonymous", contains_field(type, get_identifier("b")));

  bool should_check_duplicates = false;
  bool should_do_extra_pass = false;

  int pass = -1;

extra_pass:
  pass += 1;
  prev = &TYPE_FIELDS(type);

  while (*prev) {
    tree field = *prev;

    if (TREE_CODE(field) != FIELD_DECL) {
      prev = &DECL_CHAIN(field);
      continue;
    }

    tree flatten_struct_attr;
    if ((flatten_struct_attr =
             lookup_attribute("flatten_struct", DECL_ATTRIBUTES(field)))) {

      should_check_duplicates = true;

      tree anon_field = copy_field(field, DECL_CONTEXT(field));
      DECL_NAME(anon_field) = NULL_TREE;

      tree arguments = TREE_VALUE(flatten_struct_attr);
      if (arguments && TREE_CODE(arguments) == TREE_LIST) {
        tree ignore_duplicates = TREE_VALUE(arguments);
        if (ignore_duplicates && TREE_CODE(ignore_duplicates) == INTEGER_CST) {
          if (TREE_INT_CST_LOW(ignore_duplicates)) {
            if (pass == 0) {
              should_do_extra_pass = true;
              prev = &DECL_CHAIN(field);
              continue;
            }
            tree anon_type = copy_struct_no_duplicate(TREE_TYPE(field), type);
            TREE_TYPE(anon_field) = anon_type;
          }
        } else {
          warning(UNKNOWN_LOCATION,
                  "ignoring non-integer argument to 'flatten_struct'");
        }
      }

      bool remove_named_field = false;

      arguments = arguments ? TREE_CHAIN(arguments) : NULL_TREE;
      if (arguments) {
        tree no_named_field = TREE_VALUE(arguments);
        if (no_named_field && TREE_CODE(no_named_field) == INTEGER_CST) {
          if (TREE_INT_CST_LOW(no_named_field)) {
            remove_named_field = true;
            DECL_CHAIN(anon_field) = DECL_CHAIN(field);
          }
        } else {
          warning(UNKNOWN_LOCATION,
                  "ignoring non-integer argument to 'no_named_field'");
        }
      }

      remove_attribute(anon_field, "flatten_struct");
      remove_attribute(field, "flatten_struct");

      DECL_CHAIN(anon_field) = DECL_CHAIN(field);

      if (remove_named_field) {
        *prev = anon_field;
      } else {
        DECL_CHAIN(field) = anon_field;
      }
      prev = &DECL_CHAIN(anon_field);
      continue;
    }

    prev = &DECL_CHAIN(field);
  }

  if (should_do_extra_pass && pass == 0) {
    goto extra_pass;
  }

  if (!should_check_duplicates)
    return;

  // Final Check if we have duplicate fields
  for (tree list = get_list_fields(type); list; list = TREE_CHAIN(list)) {
    tree value = TREE_VALUE(list);
    if (!value || TREE_CODE(value) != FIELD_DECL)
      continue;

    const char *id =
        DECL_NAME(value) ? IDENTIFIER_POINTER(DECL_NAME(value)) : NULL;
    if (!id)
      continue;

    for (tree match = TREE_CHAIN(list); match; match = TREE_CHAIN(match)) {
      tree field = TREE_VALUE(match);
      if (!field || TREE_CODE(field) != FIELD_DECL)
        continue;
      const char *f_id =
          DECL_NAME(field) ? IDENTIFIER_POINTER(DECL_NAME(field)) : NULL;
      if (!f_id)
        continue;

      if (strcmp(f_id, id) == 0) {
        error("Duplicate member '%s' in 'struct %s'", id,
              TYPE_NAME(type) ? IDENTIFIER_POINTER(TYPE_NAME(type))
                              : "anonymous");
      }
    }
  }
}

static tree handle_flatten_attr(tree *node, tree name, tree args, int flags,
                                bool *no_add_attrs) {
  if (TREE_CODE(*node) != FIELD_DECL) {
    warning(UNKNOWN_LOCATION,
            "flatten_struct attribute not used on FIELD_DECL!");
    return NULL_TREE;
  }

  return NULL_TREE;
}

static struct attribute_spec flatten_attr = {
    "flatten_struct",    0,    2, true, false, false, false,
    handle_flatten_attr, NULL,
};

static void register_attributes(void *event_data, void *data) {
  register_attribute(&flatten_attr);
}

int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    return 1;
  }

  register_callback(plugin_info->base_name, PLUGIN_ATTRIBUTES,
                    register_attributes, NULL);

  register_callback(plugin_info->base_name, PLUGIN_FINISH_TYPE, finish_type,
                    NULL);

  return 0;
}
