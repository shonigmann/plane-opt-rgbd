# generated from catkin/cmake/template/pkg.context.pc.in
CATKIN_PACKAGE_PREFIX = ""
PROJECT_PKG_CONFIG_INCLUDE_DIRS = "${prefix}/include".split(';') if "${prefix}/include" != "" else []
PROJECT_CATKIN_DEPENDS = "message_runtime;rospy;roscpp".replace(';', ' ')
PKG_CONFIG_LIBRARIES_WITH_PREFIX = "-lmesh_partition;-lgflags".split(';') if "-lmesh_partition;-lgflags" != "" else []
PROJECT_NAME = "mesh_partition"
PROJECT_SPACE_DIR = "/usr/local"
PROJECT_VERSION = "0.0.0"
