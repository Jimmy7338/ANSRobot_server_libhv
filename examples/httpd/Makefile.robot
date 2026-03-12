# Makefile for Robot Backend (libhv httpd)
# 用于编译 Neural-SLAM 后端服务器

# ============================================================================
# 编译配置
# ============================================================================

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -fPIC
LDFLAGS = -L../../lib -lhv -lpthread

# 包含路径
INCLUDES = -I. -I../../ -I../../base -I../../event -I../../http -I../../http/server -I../../http/client -I../../cpputil -I../../evpp -I../../ssl -I../../util -I../../ssl -I../../util

# ============================================================================
# 源文件和目标文件
# ============================================================================

# 原始源文件
ORIGINAL_SRCS = handler.cpp router.cpp

# 新增源文件（机器人后端）
ROBOT_SRCS = device_manager.cpp robot_handler.cpp robot_router.cpp

# 所有源文件
SRCS = httpd.cpp $(ORIGINAL_SRCS) $(ROBOT_SRCS)

# 目标文件
OBJS = $(SRCS:.cpp=.o)

# 可执行文件
TARGET = httpd

# ============================================================================
# 编译规则
# ============================================================================

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Build successful: $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo "  Compiled: $<"

# ============================================================================
# 清理规则
# ============================================================================

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "✓ Clean complete"

distclean: clean
	rm -f *.o $(TARGET)

# ============================================================================
# 安装规则
# ============================================================================

install: $(TARGET)
	mkdir -p ../../../bin
	cp $(TARGET) ../../../bin/
	@echo "✓ Installed to ../../../bin/$(TARGET)"

# ============================================================================
# 调试规则
# ============================================================================

debug: CXXFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "✓ Debug build complete"

# ============================================================================
# 帮助信息
# ============================================================================

help:
	@echo "Available targets:"
	@echo "  make              - Build the robot backend"
	@echo "  make clean        - Remove object files"
	@echo "  make distclean    - Remove all generated files"
	@echo "  make install      - Install to ../../../bin/"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make help         - Show this help message"

.PHONY: all clean distclean install debug help
