#**************************************************************
#
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
#  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
#  specific language governing permissions and limitations
#  under the License.
#
#**************************************************************



$(eval $(call gb_Module_Module,comphelper))

$(eval $(call gb_Module_add_targets,comphelper,\
	Package_inc \
	Library_comphelp \
))

ifeq ($(ENABLE_UNIT_TESTS),YES)
$(eval $(call gb_Module_add_check_targets,comphelper,\
	GoogleTest_comphelper_string \
	GoogleTest_comphelper_weakbag \
))
endif

ifneq ($(OOO_JUNIT_JAR),)
$(eval $(call gb_Module_add_subsequentcheck_targets,comphelper,\
	JunitTest_comphelper_complex \
))
endif

# vim: set noet sw=4 ts=4:
