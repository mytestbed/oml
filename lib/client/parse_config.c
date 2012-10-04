/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
/*!\file parse_config.c
  \brief Implements the parsing of the configuration file.
*/

#include <log.h>
#include <mstring.h>
#include <mem.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libxml/tree.h>
#include "filter/factory.h"
#include "client.h"
#include "oml_value.h"

enum ConfToken {
  CT_ROOT,
  CT_NODE,
  CT_EXP,
  CT_COLLECT,
  CT_COLLECT_URL,
  CT_COLLECT_ENCODING,
  CT_STREAM,
  CT_STREAM_NAME,
  CT_STREAM_SOURCE,
  CT_STREAM_SAMPLES,
  CT_STREAM_INTERVAL,
  CT_FILTER,
  CT_FILTER_FIELD,
  CT_FILTER_OPER,
  CT_FILTER_RENAME,
  CT_FILTER_PROP,
  CT_FILTER_PROP_NAME,
  CT_FILTER_PROP_TYPE,
  CT_Max
};

struct synonym {
  const char *name;
  struct synonym *next;
};

static struct synonym *tokmap_[CT_Max] = {0};
static enum ConfToken curtok = CT_ROOT;

void setcurtok (enum ConfToken tok) { curtok = tok; }
void mksyn (const char * const str)
{
  struct synonym *new = xmalloc (sizeof (struct synonym));
  new->name = xstrndup (str, strlen (str));
  new->next = tokmap_[curtok];
  tokmap_[curtok] = new;
}

static void init_tokens (void)
{
  /* The canonical name goes last here, but ends up at the head of the linked list */
  setcurtok (CT_ROOT),             mksyn ("omlc");
  setcurtok (CT_NODE),             mksyn ("id");
  setcurtok (CT_EXP),              mksyn ("exp_id"), mksyn ("experiment"), mksyn ("domain");
  setcurtok (CT_COLLECT),          mksyn ("collect");
  setcurtok (CT_COLLECT_URL),      mksyn ("url");
  setcurtok (CT_COLLECT_ENCODING), mksyn ("encoding");
  setcurtok (CT_STREAM),           mksyn ("mp"), mksyn ("stream");
  setcurtok (CT_STREAM_NAME),      mksyn ("name");
  /* CT_STREAM_SOURCE is a special case */
  setcurtok (CT_STREAM_SOURCE),    mksyn ("source"), mksyn ("mp");
  setcurtok (CT_STREAM_SAMPLES),   mksyn ("samples");
  setcurtok (CT_STREAM_INTERVAL),  mksyn ("interval");
  setcurtok (CT_FILTER),           mksyn ("f"), mksyn ("filter");
  setcurtok (CT_FILTER_FIELD),     mksyn ("pname"), mksyn ("field");
  setcurtok (CT_FILTER_OPER),      mksyn ("fname"), mksyn ("operation");
  setcurtok (CT_FILTER_RENAME),    mksyn ("sname"), mksyn ("rename");
  setcurtok (CT_FILTER_PROP),      mksyn ("fp"), mksyn ("property");
  setcurtok (CT_FILTER_PROP_NAME), mksyn ("name");
  setcurtok (CT_FILTER_PROP_TYPE), mksyn ("type");
}

static struct synonym *token_names(enum ConfToken index)
{
  if (!tokmap_[0])
    init_tokens();
  if (index >= CT_Max)
    return NULL;

  return tokmap_[index];
}

/**
 * @brief Get the string value of an XML attribute identified by a
 * token.
 *
 * The token can map to multiple attribute names; the value of the
 * first one that is present will be returned.  This mechanism allows
 * synonyms for each attribute of interest, and was introduced to
 * allow easier backwards compatibility whilst 'reskinning' the XML
 * config file itself.  If none of the synonyms is present as an
 * attribute of the given XML element, NULL is returned.
 *
 * @param el the XML element to inspect for the given attribute.
 * @param tok the token representing the attribute(s) of interest.
 * @return the string value of the attribute, or NULL if not found..
 */
static char*
get_xml_attr (xmlNodePtr el, enum ConfToken tok)
{
  xmlChar *attrVal = NULL;
  const struct synonym *synonyms = token_names(tok);
  while (synonyms && !attrVal) {
    attrVal = xmlGetProp(el, (const xmlChar*)synonyms->name);
    synonyms = synonyms->next;
  }

  if (attrVal != NULL) {
    size_t len = strlen ((char*)attrVal) + 1;
    char *val = xstrndup ((char*)attrVal, len);
    xmlFree(attrVal);
    return val;
  } else {
    return NULL;
  }
}

/**
 * @brief Get the string name of an XML attribute identified by a
 * token.
 *
 * The name returned is the one used in the actual config XML file,
 * not the canonical name.
 *
 * @param el the XML element to inspect for the given attribute.
 * @param tok the token representing the attribute(s) of interest.
 * @return the name of the attribute that matches the token, if any,
 * otherwise NULL.
 */
#if 0 // unused
static const char*
get_xml_attr_name (xmlNodePtr el, enum ConfToken tok)
{
  xmlChar *attrVal = NULL;
  const struct synonym *synonyms = token_names(tok);

  while (synonyms && !attrVal) {
    attrVal = xmlGetProp(el, (const xmlChar*)synonyms->name);
    if (attrVal)
      return synonyms->name;
    synonyms = synonyms->next;
  }
  return NULL;
}
#endif

/**
 * @brief Check whether an element name matches a given token.
 *
 * If the actual name of the element matches one of the synonyms for
 * the token, return non-zero; otherwise, returns zero (false).
 *
 * @param el the element to inspect
 * @param tok the token to match against.
 * @return non-zero (true) if the element matches the token, zero
 * (false) otherwise.
 */
static int
match_xml_elt (xmlNodePtr el, enum ConfToken tok)
{
  const struct synonym *synonym = token_names(tok);

  while (synonym) {
    if (xmlStrcmp (el->name, (const xmlChar*)synonym->name) == 0)
      return 1;
    synonym = synonym->next;
  }
  return 0;
}

/**
 * @brief Get the canonical name for a token.
 *
 * The canonical name is the currently blessed "official" name for the
 * token, as it appears in the config XML file.
 *
 * @param tok the token to look up.
 * @return NULL if the token is not found or has no registered names;
 * otherwise, the official name for the token.
 */
static const char *
canonical_name (enum ConfToken tok)
{
  const struct synonym *synonym = token_names(tok);
  if (synonym)
    return synonym->name;
  else
    return NULL;
}

/**************************************************/

static int parse_collector(xmlNodePtr el);
static int parse_stream_or_mp(xmlNodePtr el, OmlWriter* writer);
static int parse_mp(xmlNodePtr el, OmlWriter* writer);
static int parse_stream(xmlNodePtr el, OmlWriter* writer);
static int parse_stream_filters (xmlNodePtr el, OmlWriter* writer,
                                 char *source, char *name);
static OmlFilter* parse_filter(xmlNodePtr el, OmlMStream* ms, OmlMP* mp);
static OmlFilter* parse_filter_properties(xmlNodePtr el, OmlFilter* f);
static int set_filter_property(OmlFilter* f, const char* pname,
                               const char* ptype, const char* pvalue);

/**************************************************/

/**
 * @brief Parse the config file to configure liboml2.
 * @param configFile the name of the config file
 * @return 0 if successful <0 otherwise
 */
int
parse_config(char* configFile)
{
  xmlDocPtr doc;
  xmlNodePtr cur;

  if ((doc = xmlParseFile(configFile)) == NULL) {
    logerror("Config file '%s' not parsed successfully.\n",
      configFile);
    return -1;
  }

  if ((cur = xmlDocGetRootElement(doc)) == NULL) {
    logerror("Config file '%s' is empty.\n", configFile);
    xmlFreeDoc(doc);
    return -2;
  }

  if (!match_xml_elt (cur, CT_ROOT)) {
    logerror("Config file has incorrect root '%s', should be '%s'.\n",
             cur->name, canonical_name (CT_ROOT));
    xmlFreeDoc(doc);
    return -3;
  }

  if (omlc_instance->node_name == NULL) {
    omlc_instance->node_name = get_xml_attr(cur, CT_NODE);
  }
  if (omlc_instance->domain == NULL) {
    omlc_instance->domain = get_xml_attr(cur, CT_EXP);
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if (match_xml_elt(cur, CT_COLLECT)) {
      if (parse_collector(cur)) {
        xmlFreeDoc(doc);
        return -4;
      }
    }
    cur = cur->next;
  }
  xmlFreeDoc(doc);
  return 0;
}

/**
 * @brief Parse the definition of a single collector.
 *
 * Extracts the URL for the collector to send its measurement streams
 * to, then parses its child elements for measurement streams to
 * build.
 *
 * @param el the XML element to analyse
 * @return 0 if successful <0 otherwise
 */
static int
parse_collector(xmlNodePtr el)
{
  char* url = get_xml_attr(el, CT_COLLECT_URL);
  if (url == NULL) {
    logerror("Config line %hu: Missing 'url' attribute for <%s ...>'.\n", el->line, el->name);
    return -1;
  }
  OmlWriter* writer;
  char* encoding_s = get_xml_attr(el, CT_COLLECT_ENCODING);
  enum StreamEncoding encoding;
  if (encoding_s == NULL || strcmp(encoding_s, "binary") == 0) {
    encoding = SE_Binary;
  } else if (strcmp(encoding_s, "text") == 0) {
    encoding = SE_Text;
  } else {
    logerror("Config line %hu: Unknown 'encoding' value '%s' for <%s ...>'.\n", el->line, encoding_s, el->name);
    return -1;
  }
  if ((writer = create_writer(url, encoding)) == NULL) return -2;

  xmlNodePtr cur = el->xmlChildrenNode;
  while (cur != NULL) {
    if (match_xml_elt (cur, CT_STREAM)) {
      if (parse_stream_or_mp(cur, writer)) {
        return -3;
      }
    }
    cur = cur->next;
  }

  return 0;
}

/**
 * @brief Parse an <mp/> or <stream/> element and build a measurement
 * stream from it.
 *
 * <mp/> is the old (badly named) format for describing a stream,
 * <stream/> is the new one.
 *
 * @param el the XML element to analyse
 * @param writer the writer to which the stream will send its output
 * @return 0 if successful <0 otherwise
 */
static int
parse_stream_or_mp (xmlNodePtr el, OmlWriter* writer)
{
  /*
   * CT_STREAM_SOURCE is a special case because we mix the old names
   *  with the new: the 'name' attribute identifies the source MP in
   *  the old <mp ...> naming, but it identifies the created _stream_
   *  in the new naming.  It requires special handling as a result.
   */
  if (xmlStrcmp (el->name, (xmlChar*)"mp") == 0)
    return parse_mp (el, writer);
  else
    return parse_stream (el, writer);
}

/**
 * @brief Parse an <mp/> element and build a measurement stream from
 * it.
 *
 * @param el the XML element to analyse
 * @param writer the writer to which the stream will send its output
 * @return 0 if successful <0 otherwise
 */
static int
parse_mp (xmlNodePtr el, OmlWriter* writer)
{
  xmlChar *source = xmlGetProp (el, (xmlChar *)"name");
  xmlChar *name   = xmlGetProp (el, (xmlChar *)"rename");

  char *source_ = xstrndup ((char*)source, strlen ((char*)source));
  char *name_;
  if (name) {
    name_ = xstrndup ((char*)name, strlen ((char*)name));
    xmlFree (name);
  } else {
    name_ = NULL;
  }
  xmlFree (source);

  return parse_stream_filters (el, writer, source_, name_);
}

/**
 * @brief Parse an <stream/> element and build a measurement stream
 * from it.
 *
 * @param el the XML element to analyse
 * @param writer the writer to which the stream will send its output
 * @return 0 if successful <0 otherwise
 */
static int
parse_stream (xmlNodePtr el, OmlWriter* writer)
{
  char *source = get_xml_attr (el, CT_STREAM_SOURCE);
  char *name   = get_xml_attr (el, CT_STREAM_NAME);

  return parse_stream_filters (el, writer, source, name);
}

/**
 * @brief Parse the <filter/> children of an <mp/> or <stream/>
 * element and construct a measurement stream together with the
 * described filters.
 *
 * @param el the XML element to analyse
 * @param writer the writer to which the stream will send its output
 * @param source the name of the source MP for this stream
 * @param the name of this stream.  If NULL, the name will be the name
 * of the source MP.
 * @return 0 if successful <0 otherwise
 */
static int
parse_stream_filters (xmlNodePtr el, OmlWriter *writer,
                      char *source, char *name)
{
  OmlMP* mp;
  char *samples_str  = get_xml_attr (el, CT_STREAM_SAMPLES);
  char *interval_str = get_xml_attr (el, CT_STREAM_INTERVAL);

  if (source == NULL) {
    logerror("Config line %hu: Missing 'name' attribute for <%s ...>'.\n",
             el->line, el->name);
    return -1;
  }

  mp = find_mp (source);

  if (mp == NULL) {
    logerror("Config line %hu: Unknown measurement point '%s'.\n", el->line, source);
    return -4;
  }

  if (samples_str == NULL && interval_str == NULL) {
    logerror("Config line %hu: Missing 'samples' or 'interval' attribute for <%s ...>'.\n",
             el->line, el->name);
    return -2;
  }
  if (samples_str != NULL && interval_str != NULL) {
    logerror("Config line %hu: Only one of 'samples' or 'interval' attribute can"
             " be defined for <%s ...>.\n",
             el->line,
             el->name);
    return -3;
  }

  int samples = samples_str != NULL ? atoi((char*)samples_str) : -1;
  if (samples == 0) samples = 1;
  double interval = interval_str != NULL ? atof((char*)interval_str) : -1;
  OmlMStream* ms = create_mstream(name, mp, writer, interval, samples);

  if (!ms)
    return -6;

  xmlNodePtr el2 = el->children;
  for (; el2 != NULL; el2 = el2->next) {
    if (match_xml_elt (el2, CT_FILTER)) {
      OmlFilter* f = parse_filter(el2, ms, mp);
      if (f == NULL) {
        // too difficult to reclaim all memory
        return -5;
      }
      f->next = ms->filters;
      ms->filters = f;
    }
  }

  // no filters specified - use default
  if (ms->filters == NULL)
    create_default_filters(mp, ms);
  ms->next = mp->streams;
  mp->streams = ms;
  if (interval > 0)
    filter_engine_start(ms);
  return 0;
}

/**
 * @brief Parse a <filter/> element and return the configured filter.
 *
 * @param el the XML element to analyze.
 * @param ms the stream to which the filter is associated.
 * @param mp the measurement point to which the stream is attached.
 * @return an OmlFilter if successful NULL otherwise
 */
static OmlFilter*
parse_filter (xmlNodePtr el, OmlMStream* ms, OmlMP* mp)
{
  OmlFilter *f = NULL;
  OmlMPDef *def = NULL;
  char* field = get_xml_attr (el, CT_FILTER_FIELD);
  char* operation = get_xml_attr (el, CT_FILTER_OPER);
  char* rename = get_xml_attr (el, CT_FILTER_RENAME);
  int index = -1;

  if (field == NULL) {
    logerror("Config line %hu: Filter config element <%s ...> must include a '%s' attribute.\n",
             el->line, el->name, canonical_name (CT_FILTER_FIELD));
      return NULL;
  }

  index = find_mp_field (field, mp);

  /* If index == -1, we didn't find the named field. This is an error, so we should abort. */
  if (index == -1) {
    MString *s = mp_fields_summary (mp);
    logerror ("Config line %hu: MP '%s' has no field named '%s'; "
              "Valid fields for '%s' are: %s\n",
              el->line, mp->name, field, mp->name, mstring_buf (s));
    mstring_delete (s);
    return NULL;
  }

  def = &mp->param_defs[index];
  if (operation == NULL) {
    // pick default one
    if (def == NULL) {
      logerror("Config line %hu: Can't create default filter without '%s' declaration.\n",
               el->line, canonical_name (CT_FILTER_FIELD));
      return NULL;
    }
    f = create_default_filter(def, ms, index);
  } else {
    const char* name = (rename != NULL) ? (char*)rename : def->name;
    f = create_filter((const char*)operation, name, def->param_types, index);
  }
  if (f != NULL) {
    f = parse_filter_properties(el, f);
  }
  return f;
}

/**
 * @brief Parse optional filter properties and call the filter's 'set'
 * funtion with the properly cast values.
 *
 * A property has a name and a type, which are specified in attributes
 * on the <property/> element, and a value, which is carried in the
 * text content of the <property/> element.  The standard OML types
 * are supported, and are specified using their schema string
 * representations (i.e. int32, uint32, int64, uint64, double, string,
 * blob, ...).
 *
 * @param el the filer XML element
 * @param f the filter to set the properties on.
 * @param ms the stream to which the filter is associated.
 * @param mp the measurement point to which the filter is associated.
 * @return an OmlFilter if successful NULL otherwise
 */
static OmlFilter*
parse_filter_properties(xmlNodePtr el, OmlFilter* f)
{
  xmlNodePtr el2 = el->children;
  for (; el2 != NULL; el2 = el2->next) {
    if (match_xml_elt (el2, CT_FILTER_PROP)) {
      if (f->set == NULL) {
        char *operation = get_xml_attr (el, CT_FILTER_OPER);
        logerror("Filter '%s' doesn't support setting properties.\n",
                 operation);
        return NULL;
      }

      char* pname = get_xml_attr (el2, CT_FILTER_PROP_NAME);
      if (pname == NULL) {
        logerror("Config line %hu: Filter property declared without a name in filter '%s'\n",
                 el2->line, f->name);
        return NULL;
      }
      char* ptype = get_xml_attr (el2, CT_FILTER_PROP_TYPE);
      if (ptype == NULL) ptype = "string";

      xmlNodePtr vel = el2->children;
      xmlChar* value = NULL;
      for (; vel != NULL; vel = vel->next) {
        if (vel->type == XML_TEXT_NODE) {
          value = vel->content;
          break;
        }
      }
      if (value == NULL) {
        logerror("Config line %hu: Missing value for property '%s' in filter '%s'.\n",
                 el2->line, pname, f->name);
        return NULL;
      }
      logdebug("Found filter property: %s:%s = '%s'.\n",
               pname, ptype, value);
      set_filter_property(f, (char*)pname, (char*)ptype, (char*)value);
    }
  }
  return f;
}

/**
 * @brief Set property a property on a filter.
 *
 * The property type should be the string representation of one of the
 * OML_*_VALUE types, as per schema header declarations.
 *
 * @param f filter to set the property for.
 * @param name Name of the property to set.
 * @param type Type of the property, as a string.
 * @param value Value to set property to
 *
 * @return 1 on success, 0 or less for failure
 */
static int
set_filter_property(OmlFilter* f, const char* name, const char* type, const char* value)
{
  OmlValue v;
  v.type = oml_type_from_s (type);

  if (oml_value_from_s (&v, value) == -1) {
    logerror("Could not convert property '%s' value from string '%s'\n",
             name, value);
    return 0;
  }

  return f->set(f, name, &v);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
