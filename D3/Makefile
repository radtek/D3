#-----------------------------------------------------------------------------#
#	Module 					: D3                                                        #
#-----------------------------------------------------------------------------#

CC = g++
CC_COMPILE = -c
CC_DEBUG = -g

OUTPUT_DIR_DEBUG = Debug_$(BUILD_TARGET)_$(ORACLE_VERSION)
OUTPUT_DIR_RELEASE = Release_$(BUILD_TARGET)_$(ORACLE_VERSION)

include ../makeopts.mk

# compile log
COMPILE_LOG = D3-Compile-Log
COMPILE_LOG_CMD = tee --append $(COMPILE_LOG)

CPPFLAGS += -I$(APALUTIL_INCDIR)
CPPFLAGS += -I$(D3_INCDIR)/$(BUILD_TARGET)
CPPFLAGS += -I$(D3_INCDIR)

# this module's objects
CPPFILES = \
Column.cpp \
D3.cpp \
D3Date.cpp \
D3Funcs.cpp \
D3MetaDatabaseBase.cpp \
D3MetaDatabase.cpp \
D3MetaEntityBase.cpp \
D3MetaEntity.cpp \
D3MetaColumnBase.cpp \
D3MetaColumn.cpp \
D3MetaColumnChoiceBase.cpp \
D3MetaColumnChoice.cpp \
D3MetaKeyBase.cpp \
D3MetaKey.cpp \
D3MetaKeyColumnBase.cpp \
D3MetaKeyColumn.cpp \
D3MetaRelationBase.cpp \
D3MetaRelation.cpp \
D3ColumnPermission.cpp \
D3ColumnPermissionBase.cpp \
D3DatabasePermission.cpp \
D3DatabasePermissionBase.cpp \
D3EntityPermission.cpp \
D3EntityPermissionBase.cpp \
D3Role.cpp \
D3RoleBase.cpp \
D3RoleUser.cpp \
D3RoleUserBase.cpp \
D3RowLevelPermission.cpp \
D3RowLevelPermissionBase.cpp \
D3Session.cpp \
D3SessionBase.cpp \
D3User.cpp \
D3UserBase.cpp \
D3Types.cpp \
Database.cpp \
Entity.cpp \
Exception.cpp \
IOField.cpp \
IOFile.cpp \
IOFileImport.cpp \
Key.cpp \
ObjectLink.cpp \
ODBCDatabase.cpp \
OTLDatabase.cpp \
Relation.cpp \
ResultSet.cpp \
Session.cpp \
XMLImporterExporter.cpp \
HSMetaColumnTopic.cpp \
HSMetaColumnTopicBase.cpp \
HSMetaDatabaseTopic.cpp \
HSMetaDatabaseTopicBase.cpp \
HSMetaEntityTopic.cpp \
HSMetaEntityTopicBase.cpp \
HSMetaKeyTopic.cpp \
HSMetaKeyTopicBase.cpp \
HSMetaRelationTopic.cpp \
HSMetaRelationTopicBase.cpp \
HSResource.cpp \
HSResourceBase.cpp \
HSResourceUsage.cpp \
HSResourceUsageBase.cpp \
HSTopic.cpp \
HSTopicBase.cpp \
HSTopicAssociation.cpp \
HSTopicAssociationBase.cpp \
HSTopicLink.cpp \
HSTopicLinkBase.cpp \
D3HistoricPasswordBase.cpp \
D3HistoricPassword.cpp

ifeq ($(BUILD_TARGET),RTCIS)
	CPPFILES += \
	$(BUILD_TARGET)/D3CustomTypes.cpp \
	$(BUILD_TARGET)/MetaProduct.cpp \
	$(BUILD_TARGET)/MetaProductGroup.cpp \
	$(BUILD_TARGET)/MetaProductPallet.cpp
endif

OBJS_DEBUG = $(CPPFILES:%.cpp=$(OUTPUT_DIR_DEBUG)/%.o)

OBJS_RELEASE = $(CPPFILES:%.cpp=$(OUTPUT_DIR_RELEASE)/%.o)

# default compile command to be used
CURRENT_DEFAULT_COMPILE = $(CC) $(CC_COMPILE) $(CC_DEBUG) $(RELEASE_OPTIMIZER) $(CPPFLAGS) $(DEBUG_CPPFLAGS) $<

DEPDIR_DEBUG = $(OUTPUT_DIR_DEBUG)/depends
DEPDIR_RELEASE = $(OUTPUT_DIR_RELEASE)/depends

DEPDIRS_DEBUG = $(OUTPUT_DIR_DEBUG)\/depends
DEPDIRS_RELEASE = $(OUTPUT_DIR_RELEASE)\/depends

MAKEDEPEND_DEBUG = $(CC) $(CC_COMPILE) $(CC_DEBUG) $(CPPFLAGS) $(DEBUG_CPPFLAGS) -M -o $(DEPDIR_DEBUG)/$(*F).d $<

MAKEDEPEND_RELEASE = $(CC) $(CC_COMPILE) $(CC_DEBUG) $(RELEASE_OPTIMIZER) $(CPPFLAGS) -M -o $(DEPDIR_RELEASE)/$(*F).d $<

df_debug = $(DEPDIR_DEBUG)/$(*F)
df_release = $(DEPDIR_RELEASE)/$(*F)

# Final binary to be produced by this build
FINAL_DEBUG = $(OUTPUT_DIR_DEBUG)/libD3.a

FINAL_RELEASE = $(OUTPUT_DIR_RELEASE)/libD3.a

# default target
all :	$(FINAL_DEBUG)
	@echo "+-----------------------------------------------------+"
	@echo "|                                                     |"
	@echo "| Module [D3] Debug Edition : Compiled successfully   |"
	@echo "|                                                     |"
	@echo "+-----------------------------------------------------+"

allRelease :	$(FINAL_RELEASE)
	@echo "+-------------------------------------------------------+"
	@echo "|                                                       |"
	@echo "| Module [D3] Release Edition : Compiled successfully   |"
	@echo "|                                                       |"
	@echo "+-------------------------------------------------------+"

$(FINAL_DEBUG) : $(OBJS_DEBUG)
	ar cr $@ $(OBJS_DEBUG)

$(FINAL_RELEASE) : $(OBJS_RELEASE)
	ar cr $@ $(OBJS_RELEASE)

$(DEPDIR_DEBUG)/%.P : %.cpp
	@ set -e; \
	echo $(df_debug).P; \
	$(MAKEDEPEND_DEBUG); \
	cp $(df_debug).d $(df_debug).P; \
	sed 's/\(.*\)\.o[ :]*/$(OUTPUT_DIR_DEBUG)\/\1.o $(DEPDIRS_DEBUG)\/\1.P : /' < $(df_debug).d > $(df_debug).P; \
	rm -f $(df_debug).d

$(DEPDIR_RELEASE)/%.P : %.cpp
	@ set -e; \
	echo $(df_release).P; \
	$(MAKEDEPEND_RELEASE); \
	cp $(df_release).d $(df_release).P; \
	sed 's/\(.*\)\.o[ :]*/$(OUTPUT_DIR_RELEASE)\/\1.o $(DEPDIRS_RELEASE)\/\1.P : /' < $(df_release).d > $(df_release).P; \
	rm -f $(df_release).d

$(OUTPUT_DIR_DEBUG)/%.o : %.cpp
	$(CURRENT_DEFAULT_COMPILE) -o $@

$(OUTPUT_DIR_RELEASE)/%.o : %.cpp
	$(CURRENT_DEFAULT_COMPILE) -o $@


ifeq ($(MAKECMDGOALS), debug)
MKDIRDEBUG:
	if test ! -d $(OUTPUT_DIR_DEBUG); \
	then \
		$(MKDIR) $(OUTPUT_DIR_DEBUG); \
	fi;
	
	if test ! -d $(OUTPUT_DIR_DEBUG)/depends; \
	then \
		$(MKDIR) $(OUTPUT_DIR_DEBUG)/depends; \
	fi;
	
	if test ! -d $(OUTPUT_DIR_DEBUG)/$(BUILD_TARGET); \
	then \
		$(MKDIR) $(OUTPUT_DIR_DEBUG)/$(BUILD_TARGET); \
	fi;
BUILD = debug
include $(CPPFILES:%.cpp=$(DEPDIR_DEBUG)/%.P) MKDIRDEBUG
endif

ifeq ($(MAKECMDGOALS), release)
MKDIRRELEASE:
	if test ! -d $(OUTPUT_DIR_RELEASE); \
	then \
		$(MKDIR) $(OUTPUT_DIR_RELEASE); \
	fi;
	
	if test ! -d $(OUTPUT_DIR_RELEASE)/depends; \
	then \
		$(MKDIR) $(OUTPUT_DIR_RELEASE)/depends; \
	fi;

	if test ! -d $(OUTPUT_DIR_RELEASE)/$(BUILD_TARGET); \
	then \
		$(MKDIR) $(OUTPUT_DIR_RELEASE)/$(BUILD_TARGET); \
	fi;
BUILD = release
include $(CPPFILES:%.cpp=$(DEPDIR_RELEASE)/%.P) MKDIRRELEASE
endif

ifeq ($(MAKECMDGOALS), debugclean)
BUILD = debugclean
endif

ifeq ($(MAKECMDGOALS), releaseclean)
BUILD = releaseclean
endif

ifeq ($(findstring $(BUILD),debug release releaseclean debugclean),)
MKDIRDEBUG:
	if test ! -d $(OUTPUT_DIR_DEBUG); \
	then \
		$(MKDIR) $(OUTPUT_DIR_DEBUG); \
	fi;
	
	if test ! -d $(OUTPUT_DIR_DEBUG)/depends; \
	then \
		$(MKDIR) $(OUTPUT_DIR_DEBUG)/depends; \
	fi;

	if test ! -d $(OUTPUT_DIR_DEBUG)/$(BUILD_TARGET); \
	then \
		$(MKDIR) $(OUTPUT_DIR_DEBUG)/$(BUILD_TARGET); \
	fi;
BUILD:=debug
include $(CPPFILES:%.cpp=$(DEPDIR_DEBUG)/%.P) MKDIRDEBUG
endif

.PHONY : debug release
debug: all

release : CC_DEBUG =

release : DEBUG_CPPFLAGS = -DNDEBUG

release : RELEASE_OPTIMIZER = -O3

release: allRelease

.PHONY	: debugclean releaseclean


.PHONY: debugclean releaseclean

debugclean:
	rm -f $(OUTPUT_DIR_DEBUG)/*.a;
	rm -f $(OUTPUT_DIR_DEBUG)/*.o;
	rm -f $(DEPDIR_DEBUG)/*.P;
	rm -f $(OUTPUT_DIR_DEBUG)/$(BUILD_TARGET)/*.o;
	rm -f $(OUTPUT_DIR_DEBUG)/$(BUILD_TARGET)/*.P;
	
releaseclean:
	rm -f $(OUTPUT_DIR_RELEASE)/*.a;
	rm -f $(OUTPUT_DIR_RELEASE)/*.o;
	rm -f $(DEPDIR_RELEASE)/*.P;
	rm -f $(OUTPUT_DIR_RELEASE)/$(BUILD_TARGET)/*.o;
	rm -f $(OUTPUT_DIR_RELEASE)/$(BUILD_TARGET)/*.P;
