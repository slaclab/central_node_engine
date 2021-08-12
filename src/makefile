CENTRAL_NODE_ENGINE_DIR=.

-include $(CENTRAL_NODE_ENGINE_DIR)/defs.mak

POSTBUILD_SUBDIRS=test

HEADERS = central_node_bypass.h \
        central_node_bypass_manager.h \
        central_node_database_tables.h \
        central_node_database.h \
	central_node_database_defs.h \
        central_node_engine.h \
        central_node_exception.h \
        central_node_yaml.h \
        log.h \
        log_wrapper.h \
        time_util.h \
        timer.h \
        buffer.h \
        queue.h \
        heartbeat.h \
        central_node_history.h \
        central_node_history_protocol.h \
        central_node_firmware.h

#DOCS = doc/README.yamlDefinition doc/README.yamlDefinition.html doc/README.configData.html doc/INSTALL.html
GENERATED_SRCS += git_version_string.h
#GENERATED_SRCS += $(DOCS)

central_node_engine_SRCS = central_node_database_tables.cc
central_node_engine_SRCS += central_node_database.cc
central_node_engine_SRCS += central_node_inputs.cc
central_node_engine_SRCS += central_node_engine.cc
central_node_engine_SRCS += central_node_bypass.cc
central_node_engine_SRCS += central_node_firmware.cc
central_node_engine_SRCS += central_node_firmware_noop.cc
central_node_engine_SRCS += central_node_history.cc
central_node_engine_SRCS += time_util.cc
central_node_engine_SRCS += timer.cc
central_node_engine_SRCS += heartbeat.cc
central_node_engine_SRCS += heartbeat_noop.cc
central_node_engine_SRCS += buffer.cc
central_node_engine_SRCS += queue.cc

DEP_HEADERS  = $(HEADERS)

STATIC_LIBRARIES_YES+=central_node_engine
SHARED_LIBRARIES_YES+=central_node_engine

-include $(CENTRAL_NODE_ENGINE_DIR)/rules.mak

doc/README.yamlDefinition:doc/README.yamlDefinition.in
	awk 'BEGIN{ FS="[ \t\"]+" } /\<YAML_KEY_/{ printf("s/%s/%s/g\n", $$2, $$3); }' cpsw_yaml_keydefs.h | sed -f - $< > $@

%.html: %
	sed -e 's/&/\&amp/g' -e 's/</\&lt/g' -e 's/>/\&gt/g' $< | sed -e '/THECONTENT/{r /dev/stdin' -e 'd}' tmpl.html > $@

test: sub-./test@test
	@true
