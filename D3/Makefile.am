##Process this file with automake to create Makefile.in
INCLUDES = -I.
INCLUDES += \
-I$(top_srcdir)/APALUtil \
-I$(srcdir) \
-I$(srcdir)/@BUILD_TARGET@

if DEBUG
AM_CPPFLAGS = -g -D_DEBUG
else
AM_CPPFLAGS = -O2 -DNDEBUG
endif

AM_CPPFLAGS += -DAPP3_@BUILD_TARGET@
AM_CPPFLAGS += -D@ORACLE_VERSION_DEFINE@

D3SOURCES = \
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


if RTCIS
D3SOURCES += \
@BUILD_TARGET@/D3CustomTypes.cpp \
@BUILD_TARGET@/MetaProduct.cpp \
@BUILD_TARGET@/MetaProductGroup.cpp \
@BUILD_TARGET@/MetaProductPallet.cpp
endif

noinst_LIBRARIES = libD3.a
libD3_a_SOURCES = ${D3SOURCES}
