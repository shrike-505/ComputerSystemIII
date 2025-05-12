DIR_REPO		?= $(realpath $(CURDIR)/../../repo)
DIR_UPSTREAM	?= $(DIR_REPO)/sys-project
DIR_BUILD		?= $(CURDIR)/build

DIR_SYN			?= $(DIR_UPSTREAM)/syn
DIR_TCL			?= $(DIR_UPSTREAM)/tcl
DIR_SIM			?= $(DIR_UPSTREAM)/sim
DIR_COSIM_IP	?= $(DIR_UPSTREAM)/ip
DIR_INCLUDE		?= $(DIR_UPSTREAM)/include
DIR_GENERAL		?= $(DIR_UPSTREAM)/general
CUSTOM_INCLUDE  ?= $(CURDIR)/include
DIR_SRC			?= $(CURDIR)/submit
DIR_KERNEL		?= $(CURDIR)/kernel
SIM_BUILD		?= $(DIR_BUILD)/verilate

DIR_TEST		?= $(DIR_UPSTREAM)/testcode
DIR_TESTCASE	?= $(DIR_TEST)/testcase
DIR_UARTCASE	?= $(DIR_TEST)/uart
DIR_CONVCASE	?= $(DIR_TEST)/conv
DIR_KERNEL_LINK ?= $(DIR_TEST)/kernel

# Verilator configuration
VERILATOR_TOP		:= Testbench
VERILATOR_SRCS		:= $(wildcard $(DIR_SIM)/*.v) $(wildcard $(DIR_SIM)/*.sv) $(wildcard $(DIR_SIM)/*.cpp) \
					   $(wildcard $(DIR_SRC)/*.v) $(wildcard $(DIR_SRC)/*.sv) \
					   $(wildcard $(DIR_GENERAL)/*.v) $(wildcard $(DIR_GENERAL)/*.sv)

VERILATOR_TFLAGS	:= -Wno-STMTDLY --timescale 1ns/10ps --trace
VERILATOR_FLAGS		:= --cc --exe --main --timing \
						--Mdir $(SIM_BUILD) --top-module $(VERILATOR_TOP) \
						-o $(VERILATOR_TOP) -I$(DIR_INCLUDE) -I$(CUSTOM_INCLUDE)\
						-CFLAGS "-DVL_DEBUG -DTOP=${VERILATOR_TOP} -std=c++17 \
						-iquote$(DIR_COSIM_IP)/include/riscv -iquote$(DIR_COSIM_IP)/include/cosim -iquote$(DIR_COSIM_IP)/include/fesvr" \
						-LDFLAGS "-L$(DIR_COSIM_IP)/lib  -l:libcosim.a -l:libriscv.a -l:libdisasm.a -l:libsoftfloat.a -l:libfdt.a -l:libfesvr.a"
VERILATOR_DEFINE	:= +define+TOP_DIR=\"$(SIM_BUILD)\" +define+VERILATE

TESTCASE ?= sample

.PHONY: verilate kernel board_sim uart conv wave clean bitstream vivado clean_vivado

verilate: $(VERILATOR_TOP)
	make -C $(DIR_TESTCASE) gen
	cp $(DIR_TESTCASE)/$(TESTCASE)/*.elf $(SIM_BUILD)/testcase.elf
	cp $(DIR_TESTCASE)/$(TESTCASE)/*.hex $(SIM_BUILD)/testcase.hex
	cd $(SIM_BUILD); ./$(VERILATOR_TOP)

kernel: $(DIR_TEST)/testcase.elf
	mkdir -p $(SIM_BUILD)
	make -C $(DIR_TEST)
	cp $(DIR_TEST)/testcase.elf $(SIM_BUILD)/testcase.elf
	cp $(DIR_TEST)/rom/rom.hex $(SIM_BUILD)/rom.hex
	cp $(DIR_TEST)/mini_sbi.hex $(SIM_BUILD)/mini_sbi.hex
	cp $(DIR_TEST)/dummy/dummy.hex $(SIM_BUILD)/dummy.hex

	verilator $(VERILATOR_TFLAGS) $(VERILATOR_FLAGS) $(VERILATOR_SRCS) $(VERILATOR_DEFINE) $(KERNEL_DEFINE)
	make -C $(SIM_BUILD) -f V$(VERILATOR_TOP).mk $(VERILATOR_TOP)
	cd $(SIM_BUILD); ./$(VERILATOR_TOP)

board_sim: $(VERILATOR_TOP)
	make -C $(DIR_TESTCASE) board
	cp $(DIR_TESTCASE)/$(TESTCASE)/*.elf $(SIM_BUILD)/testcase.elf
	cp $(DIR_TESTCASE)/$(TESTCASE)/*.hex $(SIM_BUILD)/testcase.hex
	cd $(SIM_BUILD); ./$(VERILATOR_TOP)

kernel_board_sim: $(DIR_TEST)/bootload.elf
	mkdir -p $(SIM_BUILD)
	make -C $(DIR_TEST)
	cp $(DIR_TEST)/bootload/bootload.elf $(SIM_BUILD)/testcase.elf
	cp $(DIR_TEST)/bootload/bootload.hex $(SIM_BUILD)/rom.hex
	cp $(DIR_TEST)/elf.hex $(SIM_BUILD)/elf.hex
	cp $(DIR_TEST)/dummy/dummy.hex $(SIM_BUILD)/dummy.hex

	verilator $(VERILATOR_TFLAGS) $(VERILATOR_FLAGS) $(VERILATOR_SRCS) $(VERILATOR_DEFINE) $(KERNEL_DEFINE) +define+BOARD_SIM
	make -C $(SIM_BUILD) -f V$(VERILATOR_TOP).mk $(VERILATOR_TOP)
	cd $(SIM_BUILD); ./$(VERILATOR_TOP)

uart: $(VERILATOR_TOP)
	make -C $(DIR_UARTCASE)
	cp $(DIR_UARTCASE)/*.elf $(SIM_BUILD)/testcase.elf
	cp $(DIR_UARTCASE)/*.hex $(SIM_BUILD)/testcase.hex
	cd $(SIM_BUILD); ./$(VERILATOR_TOP)

conv: $(VERILATOR_TOP) $(DIR_KERNEL_LINK)
	make -C $(DIR_CONVCASE)
	cp $(DIR_CONVCASE)/*.elf $(SIM_BUILD)/testcase.elf
	cp $(DIR_CONVCASE)/*.hex $(SIM_BUILD)/testcase.hex
	cd $(SIM_BUILD); ./$(VERILATOR_TOP)

verilate_sort:$(VERILATOR_TOP)
	make -C $(DIR_TEST)/sort
	cp $(DIR_TEST)/sort/testcase.elf $(SIM_BUILD)/testcase.elf
	cp $(DIR_TEST)/rom/rom.hex $(SIM_BUILD)/testcase.hex
	cp $(DIR_TEST)/dummy/dummy.hex $(SIM_BUILD)/elf.hex
	cp $(DIR_TEST)/sort/sort.hex $(SIM_BUILD)/mini_sbi.hex
	cd $(SIM_BUILD); ./$(VERILATOR_TOP)


$(DIR_KERNEL_LINK):
	ln -s $(DIR_KERNEL) $(DIR_TEST)

$(VERILATOR_TOP): $(VERILATOR_SRCS) $(DIR_COSIM_IP)
	mkdir -p $(SIM_BUILD)
	verilator $(VERILATOR_TFLAGS) $(VERILATOR_FLAGS) $(VERILATOR_SRCS) $(VERILATOR_DEFINE) +define+TESTCASE
	make -C $(SIM_BUILD) -f V$(VERILATOR_TOP).mk $(VERILATOR_TOP)

$(DIR_TEST)/testcase.elf: $(DIR_KERNEL_LINK)
	make -C $(DIR_TEST)

$(DIR_TEST)/bootload.elf: $(DIR_KERNEL_LINK)
	make -C $(DIR_TEST) board

wave:
	gtkwave $(SIM_BUILD)/$(VERILATOR_TOP).vcd

clean:
	rm -rf $(SIM_BUILD)
	make -C kernel clean
	make -C $(DIR_TEST) clean

# Vivado configuration
# Replace with your own path, for example:
#   If your Vivado is installed in Windows:
#   	 VIVADO_SETUP := call D:\App\Xilinx\Vivado\2022.2\settings64.bat
#   or in Linux:
#        VIVADO_SETUP := source /opt/Xilinx/Vivado/2022.2/settings64.sh
VIVADO_SETUP		:=  call D:\vivado\Vivado\2019.2\settings64.bat

CMD_PREFIX			:=	bash -c
PATH_TRANS			:=	realpath
DIR_PROJECT			?= 	$(DIR_BUILD)/project
BOARD				?=	xc7a100tcsg324-1
TOP_MODULE			?=	top

ifneq (,$(findstring microsoft,$(shell uname -a)))
WSLENV				:=	$(WSLENV):DIR_SRC/p:DIR_SYN/p:DIR_TCL/p:DIR_BUILD/p:DIR_INCLUDE/p:CUSTOM_INCLUDE/p:DIR_GENERAL/p
DIR_PROJECT			:=	$(DIR_BUILD)/project
CMD_PREFIX			:=	cmd.exe /c
PATH_TRANS			:=	wslpath -w
endif

export DIR_SRC DIR_SYN DIR_TCL DIR_PROJECT DIR_INCLUDE DIR_GENERAL CUSTOM_INCLUDE

bitstream:
	mkdir -p $(DIR_PROJECT)
	cd $(DIR_PROJECT); cp $(DIR_TCL)/vivado.tcl .; $(CMD_PREFIX) "$(VIVADO_SETUP) && set DIR_SRC && \
		vivado -mode batch -nojournal -source vivado.tcl -tclargs -top-module $(TOP_MODULE) -board $(BOARD)"

vivado:
	mkdir -p $(DIR_PROJECT)
	cd $(DIR_PROJECT); $(CMD_PREFIX) "$(VIVADO_SETUP) && vivado"

clean_vivado:
	rm -rf $(DIR_PROJECT)
