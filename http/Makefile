
srccpp = $(wildcard ./src/*.cpp)			
 
lib = vio_sdk 
lib_path = ./lib/linux/x86_64 
target = viobot_demo
inc_path = ./include

 
ALL:$(target)
 
#多个依赖可以直接在后面接着写
$(target):$(srccpp)
	g++ $^ -o $@ -I $(inc_path) -L$(lib_path) -Wl,-rpath=$(lib_path) -l$(lib)
 
clean:
	-rm -rf $(target)
 
debug:
	@echo $(srccpp)#显示src变量的内容
 
.PHONY: clean ALL

