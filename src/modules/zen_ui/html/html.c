#ifndef html_h
#define html_h

#include "zc_cstring.c"
#include "zc_map.c"
#include "zc_vector.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _range_t
{
  uint32_t pos;
  uint32_t len;
} range_t;

typedef struct _tag_t
{
  uint32_t pos;
  uint32_t len;

  uint32_t level;
  uint32_t parent;

  range_t id;
  range_t type;
  range_t class;
  range_t onclick;
} tag_t;

typedef struct _prop_t
{
  range_t class;
  range_t key;
  range_t value;
} prop_t;

char*   html_new_read(char* path);
tag_t*  html_new_parse_html(char* path);
prop_t* html_new_parse_css(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

char* html_new_read(char* path)
{
  FILE* f = fopen(path, "rb"); // CLOSE 0

  if (f)
  {
    fseek(f, 0, SEEK_END);

    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* string = CAL(fsize + 1, NULL, cstr_describe); // REL 0

    fread(string, 1, fsize, f);
    fclose(f); // CLOSE 0

    string[fsize] = 0;

    return string;
  }
  else
  {
    printf("CANNOT OPEN %s\n", path);
    return NULL;
  }
}

uint32_t count_tags(char* html)
{
  int   t = 0; // tag index
  char* c = html;
  while (*c)
  {
    if (*c == '<')
      t++;
    c++;
  }
  return t;
}

void extract_tags(char* html, tag_t* tags)
{
  uint32_t t = 0; // tag index
  uint32_t i = 0; // char index
  char*    c = html;
  while (*c)
  {
    if (*c == '<')
      tags[t].pos = i;
    else if (*c == '>')
    {
      tags[t].len = i - tags[t].pos + 1;
      t++;
    }
    i++;
    c++;
  }
}

range_t extract_string(char* str, uint32_t pos, uint32_t len)
{
  int start = 0;
  int end   = 0;
  int in    = 0;
  for (int i = pos; i < pos + len; i++)
  {
    char c = str[i];
    if (c == '"')
    {
      if (!in)
      {
        in    = 1;
        start = i;
      }
      else
      {
        in  = 0;
        end = i;
        break;
      }
    }
  }
  if (!in)
    return ((range_t){.pos = start, .len = end - start - 1});
  else
    return ((range_t){0});
}

range_t extract_value(tag_t tag, char* key, char* html)
{
  char*    start = strstr(html + tag.pos, key);
  uint32_t len   = start - (html + tag.pos);

  if (len < tag.len)
  {
    range_t range = extract_string(html, start - html, tag.len);
    return range;
  }
  return ((range_t){0});
}

void analyze_tags(char* html, tag_t* tags, uint32_t count)
{
  int l = 0; // level
  for (int i = 0; i < count; i++)
  {
    tags[i].level = l++;

    int ii = i;
    while (ii-- > 0)
    {
      if (tags[ii].level == tags[i].level - 1)
      {
        tags[i].parent = ii;
        break;
      }
    }

    tags[i].id      = extract_value(tags[i], "id=\"", html);
    tags[i].type    = extract_value(tags[i], "type=\"", html);
    tags[i].class   = extract_value(tags[i], "class=\"", html);
    tags[i].onclick = extract_value(tags[i], "onclick=\"", html);

    if (html[tags[i].pos + 1] == '/')
      l -= 2; // </div>
    if (html[tags[i].pos + tags[i].len - 2] == '/' || html[tags[i].pos + tags[i].len - 2] == '-')
      l -= 1; // />
  }
}

void tag_describe(void* p, int level)
{
  printf("html tag_t");
}

tag_t* html_new_parse_html(char* html)
{
  uint32_t cnt  = count_tags(html);
  tag_t*   tags = CAL(sizeof(tag_t) * (cnt + 1), NULL, tag_describe); // REL 0

  extract_tags(html, tags);
  analyze_tags(html, tags, cnt);

  for (int i = 0; i < cnt; i++)
  {
    tag_t t = tags[i];
    //printf("ind %i tag %.*s lvl %i par %i\n", i, t.len, html + t.pos, t.level, t.parent);
  }

  return tags;
}

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
  int      start = -1;
  char     in_l  = 0; // in line
  uint32_t index = 0;
  range_t class  = {0};
  char* c        = css;
  while (*c)
  {
    if (*c == '}' || *c == ' ' || *c == '\r' || *c == '\n')
    {
      if (c - css - 1 == start) start = c - css; // skip starting empty chars
    }
    else if (*c == '{') // class name
    {
      class.pos = start + 1;
      class.len = c - css - start - 2;
      while (*(css + class.pos + class.len) == ' ') class.len--;
      class.len++;
      start = c - css;
    }
    else if (*c == ':') // property name
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
      start                  = c - css;
      index++;
    }

    c++;
  }
}

void prop_desc(void* p, int level)
{
  printf("html prop_t");
}

prop_t* html_new_parse_css(char* css)
{
  uint32_t cnt   = count_props(css);
  prop_t*  props = CAL(sizeof(prop_t) * (cnt + 1), NULL, prop_desc); // REL 1

  analyze_classes(css, props);

  for (int i = 0; i < cnt; i++)
  {
    prop_t p = props[i];
    //printf("extracted prop %.*s %.*s %.*s\n", p.class.len, css + p.class.pos, p.key.len, css + p.key.pos, p.value.len, css + p.value.pos);
  }

  return props;
}

#endif
