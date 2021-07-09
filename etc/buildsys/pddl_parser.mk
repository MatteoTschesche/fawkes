#*****************************************************************************
#                      Makefile Build System for Fawkes
#                            -------------------
#   Created on 07 Jul 2021
#   Copyright (C) 2021 by Tarik Viehmann <viehmann@kbsg.rwth-aachen.de>
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

ifndef __buildsys_config_mk_
$(error config.mk must be included before pcl.mk)
endif

ifndef __buildsys_pddl_parser_mk_
__buildsys_pddl_parser_mk_ := 1

HAVE_PDDL_PARSER = $(if $(shell $(PKGCONFIG) --exists 'pddl_parser'; echo $${?/1/}),1,0)
ifeq ($(HAVE_PDDL_PARSER),1)
  CFLAGS_PDDL_PARSER = $(shell $(PKGCONFIG) --cflags 'pddl_parser')
  LDFLAGS_PDDL_PARSER = $(shell $(PKGCONFIG) --libs 'pddl_parser')
endif

endif # __buildsys_pddl_parser_mk_
