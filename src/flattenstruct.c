#include "gcc-plugin.h"
#include "plugin-version.h"

#include "diagnostic.h"
#include "stringpool.h"
#include "tree.h"

#include "attribs.h"

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

static void finish_type(void *event_data, void *data) {
  tree type = (tree)event_data;

  if (!type)
    return;

  if (TREE_CODE(type) != RECORD_TYPE)
    return;

  tree *prev = &TYPE_FIELDS(type);

  while (*prev) {
    tree field = *prev;

    if (TREE_CODE(field) != FIELD_DECL) {
      prev = &DECL_CHAIN(field);
      continue;
    }

    if (lookup_attribute("flatten_struct", DECL_ATTRIBUTES(field))) {
      tree replacement = copy_field(field, DECL_CONTEXT(field));
      DECL_NAME(replacement) = NULL_TREE;

      remove_attribute(replacement, "flatten_struct");
      remove_attribute(field, "flatten_struct");

      DECL_CHAIN(replacement) = DECL_CHAIN(field);
      DECL_CHAIN(field) = replacement;

      *prev = field;
      continue;
    }

    prev = &DECL_CHAIN(field);
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
    "flatten_struct",    0,    0, false, false, false, false,
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
