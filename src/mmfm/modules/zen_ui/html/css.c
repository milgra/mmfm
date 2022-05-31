#ifndef css_h
#define css_h

#include "zc_cstring.c"
#include "zc_map.c"
#include "zc_vector.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _css_range_t
{
  uint32_t pos;
  uint32_t len;
} css_range_t;

typedef struct _prop_t
{
  css_range_t class;
  css_range_t key;
  css_range_t value;
} prop_t;

map_t* css_new(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "cstr_util.c"

uint32_t count_props(char* css)
{
  int   t = 0; // tag index
  char* c = css;
  while (*c)
  {
    if (*c == ':')
      t++;
    c++;
  }
  return t;
}

void analyze_classes(char* css, prop_t* props)
{
  int start = -1;
  // char     in_l  = 0; // in line
  uint32_t index    = 0;
  css_range_t class = {0};
  int   in_str      = 0;
  char* c           = css;
  while (*c)
  {
    if (*c == '}' || *c == ' ' || *c == '\r' || *c == '\n')
    {
      if (c - css - 1 == start) start = c - css; // skip starting empty chars
    }
    else if (*c == '"') // class name
    {
      in_str = 1;
    }
    else if (*c == '{') // class name
    {
      class.pos = start + 1;
      class.len = c - css - start - 2;
      while (*(css + class.pos + class.len) == ' ') class.len--;
      class.len++;
      start = c - css;
    }
    else if (*c == ':' && !in_str) // property name
    {
      props[index].class   = class;
      props[index].key.pos = start + 1;
      props[index].key.len = c - css - start - 1;
      start                = c - css;
    }
    else if (*c == ';') // value name
    {
      props[index].class     = class;
      props[index].value.pos = start + 1;
      props[index].value.len = c - css - start - 1;

      if (in_str)
      {
        props[index].value.pos += 1;
        props[index].value.len -= 2;
        in_str = 0;
      }

      start = c - css;
      index++;
    }

    c++;
  }
}

void prop_desc(void* p, int level)
{
  printf("html prop_t");
}

prop_t* css_new_parse(char* css)
{
  uint32_t cnt   = count_props(css);
  prop_t*  props = CAL(sizeof(prop_t) * (cnt + 1), NULL, prop_desc); // REL 1

  analyze_classes(css, props);

  for (int i = 0; i < cnt; i++)
  {
    // prop_t p = props[i];
    // printf("extracted prop %.*s %.*s %.*s\n", p.class.len, css + p.class.pos, p.key.len, css + p.key.pos, p.value.len, css + p.value.pos);
  }

  return props;
}

map_t* css_new(char* filepath)
{
  char*   css         = cstr_new_file(filepath); // REL 0
  prop_t* view_styles = css_new_parse(css);      // REL 1
  map_t*  styles      = MNEW();                  // REL 2
  prop_t* props       = view_styles;

  while ((*props).class.len > 0)
  {
    prop_t t   = *props;
    char*  cls = CAL(sizeof(char) * t.class.len + 1, NULL, cstr_describe); // REL 3
    char*  key = CAL(sizeof(char) * t.key.len + 1, NULL, cstr_describe);   // REL 4
    char*  val = CAL(sizeof(char) * t.value.len + 1, NULL, cstr_describe); // REL 5

    memcpy(cls, css + t.class.pos, t.class.len);
    memcpy(key, css + t.key.pos, t.key.len);
    memcpy(val, css + t.value.pos, t.value.len);

    map_t* style = MGET(styles, cls);
    if (style == NULL)
    {
      style = MNEW(); // REL 6
      MPUT(styles, cls, style);
      REL(style); // REL 6
    }
    MPUT(style, key, val);
    props += 1;
    REL(cls); // REL 3
    REL(key); // REL 4
    REL(val); // REL 5
  }

  REL(view_styles);
  REL(css);

  return styles;
}

#endif
