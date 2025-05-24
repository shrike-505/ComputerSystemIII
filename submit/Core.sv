`include "core_struct.vh"
`include "mem_ift.vh"
`include "csr_struct.vh"
module Core (
    input clk,
    input rst,
    input time_int,
    input CsrPack::ExceptPack except_mmu,

    Mem_ift.Master imem_ift,
    Mem_ift.Master dmem_ift,
    output CorePack::data_t satp, // Done
    output logic [1:0] output_priv, // TBD
    output CorePack::data_t pc_if, // TBD
    output CorePack::data_t pc_mem, // TBD

    output cosim_valid,
    output CorePack::CoreInfo cosim_core_info,
    output CsrPack::CSRPack cosim_csr_info,
    output cosim_interrupt,
    output cosim_switch_mode,
    output CorePack::data_t cosim_cause
);
    import CorePack::*;
    import CsrPack::*;
    logic [63:0] inst_unselected;   // unselected instruction
    logic [31:0] inst;             // selected instruction
    logic npc_sel;
    logic [63:0] ro_addr;
    logic br_stall;

    import CsrPack::ExceptPack;
    ExceptPack except_id;
    ExceptPack except_exe;
    ExceptPack except0;
    assign except0.except = 0;
    assign except0.epc = 0;
    assign except0.ecause = 0;
    assign except0.etval = 0;
    logic except_happen_id;
    logic switch_mode;
    logic [63:0] pc_ret;
    CorePack::data_t pc_csr;


    logic                we_csr;
    logic [1 :0]         csr_ret;
    csr_alu_op_enmu      csr_alu_op;
    csr_alu_asel_op_enum csr_alu_asel;
    csr_alu_bsel_op_enum csr_alu_bsel;
    data_t               csr_val;
    data_t               csr_val_wb;
    csr_reg_ind_t        csr_addr;
    logic [2:0]          csr_sel;
    logic [1:0]          priv;


    // ***** 阶段寄存器定义 *****
    // IF/ID
    logic [63:0] if_id_pc;
    logic [31:0] if_id_inst;

    // ID/EX
    logic [63:0]         id_ex_pc;
    logic [31:0]         id_ex_inst;
    logic [63:0]         id_ex_read_data_1;
    logic [63:0]         id_ex_read_data_2;
    logic [63:0]         id_ex_imm;
    logic [4 :0]         id_ex_rs1, id_ex_rs2, id_ex_rd;
    logic [3 :0]         id_ex_alu_op;
    logic [2 :0]         id_ex_cmp_op;
    logic [1 :0]         id_ex_alu_asel, id_ex_alu_bsel;
    logic                id_ex_we_reg, id_ex_we_mem, id_ex_re_mem, id_ex_is_j;
    logic [1 :0]         id_ex_wb_sel;
    logic [2 :0]         id_ex_mem_op;
    logic [3 :0]         id_ex_memdata_width;
    logic                id_ex_we_csr;
    logic [1 :0]         id_ex_csr_ret;
    csr_alu_op_enmu      id_ex_csr_alu_op;
    csr_alu_asel_op_enum id_ex_csr_alu_asel;
    csr_alu_bsel_op_enum id_ex_csr_alu_bsel;
    data_t               id_ex_csr_val;
    csr_reg_ind_t        id_ex_csr_addr;
    logic [2:0]          id_ex_csr_sel;

    // EX/MEM
    logic [63:0] ex_mem_pc;
    logic [63:0] ex_mem_alu_res;
    logic [31:0] ex_mem_inst;
    logic [63:0] ex_mem_read_data_2;
    logic [4 :0] ex_mem_rd;
    logic ex_mem_we_reg, ex_mem_we_mem, ex_mem_re_mem;
    logic [1 :0] ex_mem_wb_sel;
    logic [2 :0] ex_mem_mem_op;
    logic [3 :0] ex_mem_memdata_width;
    logic        ex_mem_we_csr;
    logic [1:0]  ex_mem_csr_ret;
    data_t       ex_mem_csr_val_wb;
    data_t       ex_mem_csr_val;
    csr_reg_ind_t ex_mem_csr_addr;
    ExceptPack   ex_mem_except_exe;

    // MEM/WB
    logic [63:0] mem_wb_alu_res;
    logic [63:0] mem_wb_mem_data;
    logic [31:0] mem_wb_inst;
    logic [63:0] mem_wb_pc;
    logic [4 :0] mem_wb_rd;
    logic mem_wb_we_reg, mem_wb_we_mem, mem_wb_re_mem;
    logic [1 :0] mem_wb_wb_sel;
    logic [63:0] mem_wb_trunc_data;
    logic        mem_wb_we_csr;
    logic [1:0]  mem_wb_csr_ret;
    data_t       mem_wb_csr_val_wb;
    data_t       mem_wb_csr_val; // Maybe not used
    csr_reg_ind_t mem_wb_csr_addr;
    ExceptPack   mem_wb_except_exe;
    

    // ***** IF 阶段 - Instruction Fetch *****
    logic [63:0] pc, next_pc;
    parameter pc_increment = 4;

    always_comb begin
        if (cmp_res | id_ex_is_j) begin
            next_pc = alu_res;
        end else if(switch_mode) begin
            next_pc = pc_csr;
        end else if(stored_switch_mode) begin
            next_pc = stored_pc_csr;
        end else begin
            next_pc = pc + pc_increment; // 暂时没有分支跳转逻辑，始终顺序执行
        end
        ro_addr = pc;
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            pc <= 64'b0;
            if_id_pc <= 64'b0;
            if_id_inst <= 0;
        end else begin
            if (flush) begin
                pc <= next_pc;
                if_id_pc <= 64'b0; // flush IF/ID
                if_id_inst <= 0;
            end else if (stall | mem_stall | br_stall | switch_mode_stall) begin
                pc <= pc;
                if_id_pc <= if_id_pc;
                if_id_inst <= if_id_inst;
            end else if (if_stall) begin
                pc <= pc;
                if_id_pc <= 0;
                if_id_inst <= 0;
            end else begin
                pc <= next_pc;
                if_id_pc <= pc;  // 将pc传递到ID阶段
                if_id_inst <= inst;
            end
        end
    end

    always_comb begin
        if (pc[2] == 1) begin
            inst = inst_unselected[63:32];
        end else begin
            inst = inst_unselected[31:0]; // inst 传递到ID阶段
        end
    end

    // ***** ID 阶段 - Instruction Decode *****
    logic [4:0]  rs1, rs2, rd;
    logic [63:0] read_data_1, read_data_2;
    logic [63:0] Imm;
    logic [3 :0] alu_op;
    logic [2 :0] cmp_op;
    logic [1 :0] alu_asel, alu_bsel;
    logic        we_reg, we_mem, re_mem, is_j;
    logic [1 :0] wb_sel;
    logic [2 :0] mem_op;
    logic [3 :0] memdata_width;
    imm_op_enum  immgen_op;


    always_comb begin
        rs1 = if_id_inst[19:15];
        rs2 = if_id_inst[24:20];
        rd  = if_id_inst[11:7];
    end
    // Immediate Generation
    ImmGen immgen(.inst(if_id_inst),
                  .immgen_op(immgen_op),
                  .Imm(Imm)
    );


    // 寄存器读写控制和操作码解析
    Controller ctrl(
        .inst(if_id_inst),
        .we_reg(we_reg),
        .we_mem(we_mem),
        .re_mem(re_mem),
        .is_b(),
        .is_j(is_j),
        .immgen_op(immgen_op),
        .alu_op(alu_op),
        .cmp_op(cmp_op),
        .alu_asel(alu_asel),
        .alu_bsel(alu_bsel),
        .wb_sel(wb_sel),
        .mem_op(mem_op),
        .memdata_width(memdata_width),

        .csr_ret(csr_ret),
        .csr_sel(csr_sel),
        .csr_we_wb(we_csr),
        .csr_addr_id(csr_addr),
        .csr_alu_op(csr_alu_op),
        .csr_alu_asel(csr_alu_asel),
        .csr_alu_bsel(csr_alu_bsel)
    );

    logic valid_wb = mem_wb_inst!=0;
    CSRModule csr(
        .clk(clk),
        .rst(rst),
        .csr_we_wb(mem_wb_we_csr), // Done
        .csr_addr_wb(mem_wb_csr_addr), // Done
        .csr_val_wb(mem_wb_csr_val_wb), // DOne
        .csr_addr_id(csr_addr), // Done
        .csr_val_id(csr_val), // Done

        .pc_wb(pc_ret),
        .valid_wb(valid_wb),
        .time_int(time_int),
        .csr_ret(mem_wb_csr_ret),
        .except_commit(mem_wb_except_exe),
        .except_mmu(except_mmu), // TBD
        .priv(priv),
        .switch_mode(switch_mode),
        .pc_csr(pc_csr),
        .satp(satp),
        .cosim_interrupt(cosim_interrupt),
        .cosim_cause(cosim_cause),
        .cosim_csr_info(cosim_csr_info)    
        
    );

    // Register File
    RegFile regf(
        .clk(clk),
        .rst(rst),
        .we(mem_wb_we_reg),
        .read_addr_1(rs1),
        .read_addr_2(rs2),
        .write_addr(mem_wb_rd),
        .write_data(wb_val),
        .read_data_1(read_data_1),
        .read_data_2(read_data_2)
    );

    logic valid_id = (if_id_inst!=0);
    IDExceptExamine idexcept(
        .clk(clk),
        .rst(rst),
        .stall(1'b0),
        .flush(flush),
        .pc_id(if_id_pc),
        .priv(priv),
        .inst_id(if_id_inst),
        .valid_id(valid_id),

        .except_id(except_id),
        .except_exe(except_exe),
        .except_happen_id(except_happen_id)
    );

    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            id_ex_pc <= 64'b0;
            id_ex_inst <= 32'b0;  // 清空指令
            id_ex_read_data_1 <= 64'b0;
            id_ex_read_data_2 <= 64'b0;
            id_ex_imm <= 64'b0;
            id_ex_rs1 <= 5'b0;
            id_ex_rs2 <= 5'b0;
            id_ex_rd <= 5'b0;
            id_ex_alu_op <= ALU_DEFAULT;
            id_ex_cmp_op <= CMP_NO;
            id_ex_alu_asel <= ASEL0;
            id_ex_alu_bsel <= BSEL0;
            id_ex_we_reg <= 1'b0;
            id_ex_we_mem <= 1'b0;
            id_ex_re_mem <= 1'b0;
            id_ex_is_j <= 1'b0;
            id_ex_wb_sel <= WB_SEL0;
            id_ex_mem_op <= MEM_NO;
            id_ex_memdata_width <= 4'b0;
            id_ex_we_csr <= 0;
            id_ex_csr_ret <= 0;
            id_ex_csr_alu_op <= CSR_ALU_ADD;
            id_ex_csr_alu_asel <= ASEL_CSR0;
            id_ex_csr_alu_bsel <= BSEL_CSR0;
            id_ex_csr_val <= 0;
            id_ex_csr_addr <= 0;
            id_ex_csr_sel <= 3'b000;
        end else begin
            if (flush | stall | switch_mode_stall) begin
                id_ex_pc <= 64'b0;
                id_ex_inst <= 32'b0;  // 清空指令
                id_ex_read_data_1 <= 64'b0;
                id_ex_read_data_2 <= 64'b0;
                id_ex_imm <= 64'b0;
                id_ex_rs1 <= 5'b0;
                id_ex_rs2 <= 5'b0;
                id_ex_rd <= 5'b0;
                id_ex_alu_op <= ALU_DEFAULT;
                id_ex_cmp_op <= CMP_NO;
                id_ex_alu_asel <= ASEL0;
                id_ex_alu_bsel <= BSEL0;
                id_ex_we_reg <= 1'b0;
                id_ex_we_mem <= 1'b0;
                id_ex_re_mem <= 1'b0;
                id_ex_is_j <= 1'b0;
                id_ex_wb_sel <= WB_SEL0;
                id_ex_mem_op <= MEM_NO;
                id_ex_memdata_width <= 4'b0;
                id_ex_we_csr <= 0;
                id_ex_csr_ret <= 0;
                id_ex_csr_alu_op <= CSR_ALU_ADD;
                id_ex_csr_alu_asel <= ASEL_CSR0;
                id_ex_csr_alu_bsel <= BSEL_CSR0;
                id_ex_csr_val <= 0;
                id_ex_csr_addr <= 0;
                id_ex_csr_sel <= 3'b000;
            end else if (mem_stall | br_stall) begin
                id_ex_pc <= id_ex_pc;
                id_ex_inst <= id_ex_inst;
                id_ex_read_data_1 <= id_ex_read_data_1;
                id_ex_read_data_2 <= id_ex_read_data_2;
                id_ex_imm <= id_ex_imm;
                id_ex_rs1 <= id_ex_rs1;
                id_ex_rs2 <= id_ex_rs2;
                id_ex_rd <= id_ex_rd;
                id_ex_alu_op <= id_ex_alu_op;
                id_ex_cmp_op <= id_ex_cmp_op;
                id_ex_alu_asel <= id_ex_alu_asel;
                id_ex_alu_bsel <= id_ex_alu_bsel;
                id_ex_we_reg <= id_ex_we_reg;
                id_ex_we_mem <= id_ex_we_mem;
                id_ex_re_mem <= id_ex_re_mem;
                id_ex_is_j <= id_ex_is_j;
                id_ex_wb_sel <= id_ex_wb_sel;
                id_ex_mem_op <= id_ex_mem_op;
                id_ex_memdata_width <= id_ex_memdata_width;
                id_ex_we_csr <= id_ex_we_csr;
                id_ex_csr_ret <= id_ex_csr_ret;
                id_ex_csr_alu_op <= id_ex_csr_alu_op;
                id_ex_csr_alu_asel <= id_ex_csr_alu_asel;
                id_ex_csr_alu_bsel <= id_ex_csr_alu_bsel;
                id_ex_csr_val <= id_ex_csr_val;
                id_ex_csr_addr <= id_ex_csr_addr;
                id_ex_csr_sel <= id_ex_csr_sel;
            end else if(except_happen_id) begin
                id_ex_pc <= if_id_pc;
                id_ex_inst <= if_id_inst;
                id_ex_read_data_1 <= 64'b0;
                id_ex_read_data_2 <= 64'b0;
                id_ex_imm <= 64'b0;
                id_ex_rs1 <= 5'b0;
                id_ex_rs2 <= 5'b0;
                id_ex_rd <= 5'b0;
                id_ex_alu_op <= ALU_DEFAULT;
                id_ex_cmp_op <= CMP_NO;
                id_ex_alu_asel <= ASEL0;
                id_ex_alu_bsel <= BSEL0;
                id_ex_we_reg <= 1'b0;
                id_ex_we_mem <= 1'b0;
                id_ex_re_mem <= 1'b0;
                id_ex_is_j <= 1'b0;
                id_ex_wb_sel <= WB_SEL0;
                id_ex_mem_op <= MEM_NO;
                id_ex_memdata_width <= 4'b0;
                id_ex_we_csr <= 0;
                id_ex_csr_ret <= 0;
                id_ex_csr_alu_op <= CSR_ALU_ADD;
                id_ex_csr_alu_asel <= ASEL_CSR0;
                id_ex_csr_alu_bsel <= BSEL_CSR0;
                id_ex_csr_val <= 0;
                id_ex_csr_addr <= 0;
                id_ex_csr_sel <= 3'b000;
            end else begin
                // 正常寄存器传递
                id_ex_pc <= if_id_pc;
                id_ex_inst <= if_id_inst;
                id_ex_read_data_1 <= real_read_data_1;
                id_ex_read_data_2 <= real_read_data_2;
                id_ex_imm <= Imm;
                id_ex_rs1 <= rs1;
                id_ex_rs2 <= rs2;
                id_ex_rd <= rd;
                id_ex_alu_op <= alu_op;
                id_ex_cmp_op <= cmp_op;
                id_ex_alu_asel <= alu_asel;
                id_ex_alu_bsel <= alu_bsel;
                id_ex_we_reg <= we_reg;
                id_ex_we_mem <= we_mem;
                id_ex_re_mem <= re_mem;
                id_ex_is_j <= is_j;
                id_ex_wb_sel <= wb_sel;
                id_ex_mem_op <= mem_op;
                id_ex_memdata_width <= memdata_width;
                id_ex_we_csr <= we_csr;
                id_ex_csr_ret <= csr_ret;
                id_ex_csr_alu_op <= csr_alu_op;
                id_ex_csr_alu_asel <= csr_alu_asel;
                id_ex_csr_alu_bsel <= csr_alu_bsel;
                id_ex_csr_val <= csr_val;
                id_ex_csr_addr <= csr_addr;
                id_ex_csr_sel <= csr_sel;
            end
        end
    end
    
    always_comb begin
        br_stall = (cmp_res | id_ex_is_j) & (current_state != IDLE) & (current_state != MEM1);
        // flush = cmp_res | id_ex_is_j;
        // CANNOT directly flush if we are interacting with memory!
        flush    = (cmp_res | id_ex_is_j | switch_mode | stored_switch_mode) & (current_state == IDLE | current_state == MEM1);
    end

    // Need to stall if_id registers, if current time clock or the former clock need to flush
    logic switch_mode_stall;
    always_comb begin
        if (flush) begin
            switch_mode_stall = 0;
        end else if (switch_mode | stored_switch_mode) begin
            switch_mode_stall = 1;
        end else begin
            switch_mode_stall = 0;
        end
    end

    logic stored_switch_mode;
    data_t stored_pc_csr;
    always_ff @(posedge clk or posedge rst) begin
        if(switch_mode) begin
            stored_switch_mode <= switch_mode;
            stored_pc_csr      <= pc_csr;
        end else if(flush) begin
            stored_pc_csr      <= 64'b0;
            stored_switch_mode <= 0;
        end
    end

    // ***** Forwarding And Stall *****
    logic forward_a;
    logic forward_b;
    logic [63:0] rs1_sel;
    logic [63:0] rs2_sel;
    logic stall;
    ForwardingUnit forward(
        .rs1(rs1),
        .rs2(rs2),
        .id_ex_rd(id_ex_rd),
        .ex_mem_rd(ex_mem_rd),
        .mem_wb_rd(mem_wb_rd),
        .id_ex_we_reg(id_ex_we_reg),
        .ex_mem_we_reg(ex_mem_we_reg),
        .mem_wb_we_reg(mem_wb_we_reg),
        .alu_res(alu_res),
        .ex_mem_alu_res(ex_mem_alu_res),
        .wb_val(wb_val),
        .mem_wb_re_mem(mem_wb_re_mem),
        .mem_wb_trunc_data(mem_wb_trunc_data),
        .rs1_sel(rs1_sel),
        .rs2_sel(rs2_sel),
        .forward_a(forward_a),
        .forward_b(forward_b)
    );
    logic [63:0] real_read_data_1;
    logic [63:0] real_read_data_2;
    ForwardingMux forwardmux(
        .forward_a(forward_a),
        .forward_b(forward_b),
        .read_data_1(read_data_1),
        .read_data_2(read_data_2),
        .rs1_sel(rs1_sel),
        .rs2_sel(rs2_sel),
        .real_read_data_1(real_read_data_1),
        .real_read_data_2(real_read_data_2)
    );

    HazardDetection HD (
        .rs1(rs1),
        .rs2(rs2),
        .id_ex_rd(id_ex_rd),
        .ex_mem_rd(ex_mem_rd),
        .id_ex_re_mem(id_ex_re_mem),
        .ex_mem_re_mem(ex_mem_re_mem),
        .stall(stall)
    );


    // ***** EX 阶段 - Execute *****
    logic [63:0] alu_a, alu_b, alu_res;
    logic [63:0] csr_alu_a, csr_alu_b; // csr_alu_res is csr_val_wb
    logic flush, cmp_res;
    // ALU操作数选择
    always_comb begin
        case (id_ex_alu_asel)
            ASEL0:    alu_a = 64'b0;
            ASEL_REG: alu_a = id_ex_read_data_1;
            ASEL_PC:  alu_a = id_ex_pc;
            ASEL_CSR: alu_a = id_ex_csr_val;
            default:  alu_a = 64'b0;
        endcase

        case (id_ex_alu_bsel)
            BSEL0:    alu_b = 64'b0;
            BSEL_REG: alu_b = id_ex_read_data_2;
            BSEL_IMM: alu_b = id_ex_imm;
            default:  alu_b = 64'b0;
        endcase
    end

    // ALU 计算
    ALU alupart(.a(alu_a),
                 .b(alu_b),
                 .alu_op(id_ex_alu_op),
                 .res(alu_res)
    );

    // CSR 操作数计算
    always_comb begin
        case (id_ex_csr_alu_asel)
            ASEL_CSR0: csr_alu_a = 0;
            ASEL_CSRREG: csr_alu_a = id_ex_csr_val;
            default: csr_alu_a = 0;
        endcase

        case (id_ex_csr_alu_bsel)
            BSEL_CSR0: csr_alu_b = 0;
            BSEL_GPREG: csr_alu_b = id_ex_read_data_1;
            BSEL_CSRIMM: csr_alu_b = id_ex_imm;
            default: csr_alu_b = 0;
        endcase
    end

    // CSR ALU 计算
    CSRALU csr_alupart(
        .csr_a(csr_alu_a),
        .csr_b(csr_alu_b),
        .csr_alu_op(id_ex_csr_alu_op),
        .csr_alu_res(csr_val_wb)
    );

    // 分支比较器
    Cmp BranchCmp(.a(id_ex_read_data_1),
            .b(id_ex_read_data_2),
            .cmp_op(id_ex_cmp_op),
            .cmp_res(cmp_res)
    );
    
    // EX/MEM 寄存器传递
    always_ff @(posedge clk or posedge rst) begin
        if (rst | br_stall) begin
            ex_mem_pc <= 64'b0;
            ex_mem_alu_res <= 64'b0;
            ex_mem_inst <= 32'b0;
            ex_mem_read_data_2 <= 64'b0;
            ex_mem_rd <= 5'b0;
            ex_mem_we_reg <= 1'b0;
            ex_mem_we_mem <= 1'b0;
            ex_mem_re_mem <= 1'b0;
            ex_mem_wb_sel <= 2'b0;
            ex_mem_mem_op <= 3'b0;
            ex_mem_memdata_width <= 4'b0;
            ex_mem_csr_addr <= 0;
            ex_mem_csr_val_wb <= 0;
            ex_mem_csr_val <= 0;
            ex_mem_csr_ret <= 0;
            ex_mem_we_csr <= 0;
            ex_mem_except_exe <= except0;
        end else if (mem_stall) begin
            ex_mem_pc <= ex_mem_pc;
            ex_mem_alu_res <= ex_mem_alu_res;
            ex_mem_inst <= ex_mem_inst;
            ex_mem_read_data_2 <= ex_mem_read_data_2;
            ex_mem_rd <= ex_mem_rd;
            ex_mem_we_reg <= ex_mem_we_reg;
            ex_mem_we_mem <= ex_mem_we_mem;
            ex_mem_re_mem <= ex_mem_re_mem;
            ex_mem_wb_sel <= ex_mem_wb_sel;
            ex_mem_mem_op <= ex_mem_mem_op;
            ex_mem_memdata_width <= ex_mem_memdata_width; 
            ex_mem_csr_addr <= ex_mem_csr_addr;
            ex_mem_csr_val_wb <= ex_mem_csr_val_wb;
            ex_mem_csr_val <= ex_mem_csr_val;
            ex_mem_csr_ret <= ex_mem_csr_ret;
            ex_mem_we_csr <= ex_mem_we_csr;
            ex_mem_except_exe <= ex_mem_except_exe;
        end else begin
            ex_mem_pc            <= id_ex_pc;
            ex_mem_alu_res       <= alu_res;
            ex_mem_inst          <= id_ex_inst;
            ex_mem_read_data_2   <= id_ex_read_data_2;
            ex_mem_rd            <= id_ex_rd;
            ex_mem_we_reg        <= id_ex_we_reg;
            ex_mem_we_mem        <= id_ex_we_mem;
            ex_mem_re_mem        <= id_ex_re_mem;
            ex_mem_wb_sel        <= id_ex_wb_sel;
            ex_mem_mem_op        <= id_ex_mem_op;
            ex_mem_memdata_width <= id_ex_memdata_width;
            ex_mem_csr_addr      <= id_ex_csr_addr;
            ex_mem_csr_val_wb    <= csr_val_wb;
            ex_mem_csr_val       <= id_ex_csr_val;
            ex_mem_csr_ret       <= id_ex_csr_ret;
            ex_mem_we_csr        <= id_ex_we_csr;
            ex_mem_except_exe    <= except_exe;
        end
    end

    // ***** MEM 阶段 - Memory Access *****

    // Data Package, with Mask
    logic [63:0] trunc_data;
    logic [63:0] wdata_data;
    logic [7 :0] wdata_mask;
    integer loc = {29'b0, ex_mem_alu_res[2:0]};
    DataPackage data_pack(
        .alu_res(ex_mem_alu_res),
        .memdata_width(ex_mem_memdata_width),
        .read_data(ex_mem_read_data_2),
        .wdata_data_packaged(wdata_data),
        .wdata_mask_packaged(wdata_mask)
    );

    // Data Memory And Instruction Memory
    typedef enum logic [2:0] {
        IDLE = 3'b000,
        IF1  = 3'b001,
        IF2  = 3'b010,
        WAITFOR1 = 3'b011,
        WAITFOR2 = 3'b100,
        MEM1 = 3'b101,
        MEM2 = 3'b110
    } state_t;

    state_t current_state, next_state;

    // 状态机逻辑
    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            current_state <= IF1;
        end else begin
            if (flush) begin
                current_state <= IF1;
            end else begin
                current_state <= next_state;
            end
        end
        if (current_state == MEM2 && dmem_ift.r_reply_valid && dmem_ift.r_reply_ready) begin
            read_data <= dmem_ift.r_reply_bits.rdata;
        end
        if((current_state == IF2 || current_state == WAITFOR2) && imem_ift.r_reply_valid && imem_ift.r_reply_ready) begin
            inst_unselected <= imem_ift.r_reply_bits.rdata;
        end
    end

    // 信号初始化
    logic if_stall;
    logic mem_stall;
    logic [63:0] read_data;
    // 计算下一个状态
    always_comb begin
        dmem_ift.r_request_bits.raddr = ex_mem_alu_res;
        dmem_ift.w_request_bits.waddr = ex_mem_alu_res;
        dmem_ift.w_request_bits.wdata = wdata_data;
        dmem_ift.w_request_bits.wmask = wdata_mask;
        case (current_state)
            IDLE: begin
                next_state = IF1; // 无条件转移到 IF1
            end

            IF1: begin
                if (imem_ift.r_request_valid & imem_ift.r_request_ready) begin
                    next_state = IF2; // 握手成功，转移到 IF2
                end else if (re_mem || we_mem) begin
                    next_state = WAITFOR1; // 转移到 WAITFOR1
                end else begin
                    next_state = current_state;
                end
            end

            IF2: begin
                if (imem_ift.r_reply_valid & imem_ift.r_reply_ready) begin
                    next_state = IDLE; // 握手成功，转移到 IDLE
                end else begin
                    next_state = current_state;
                end
            end

            WAITFOR1: begin
                if (imem_ift.r_request_valid & imem_ift.r_request_ready) begin
                    next_state = WAITFOR2; // 握手成功，转移到 WAITFOR2
                end else begin
                    next_state = current_state;
                end
            end

            WAITFOR2: begin
                if (imem_ift.r_reply_valid & imem_ift.r_reply_ready) begin
                    next_state = MEM1; // 握手成功，转移到 MEM1
                end else begin
                    next_state = current_state;
                end
            end

            MEM1: begin
                if (dmem_ift.r_request_valid & dmem_ift.r_request_ready || dmem_ift.w_request_valid && dmem_ift.w_request_ready) begin
                    next_state = MEM2; // 握手成功，转移到 MEM2
                end else begin
                    next_state = current_state;
                end
            end

            MEM2: begin
                if (dmem_ift.r_reply_valid & dmem_ift.r_reply_ready || dmem_ift.w_reply_ready && dmem_ift.w_reply_valid) begin
                    next_state = IDLE; // 握手成功，转移到 IDLE
                end else begin
                    next_state = current_state;
                end
            end
            default: next_state = IDLE; // 默认回到 IDLE
        endcase
    end

    always_comb begin
        if (current_state == IF1 || current_state == IF2 || current_state == WAITFOR1 || current_state == WAITFOR2) begin
            if_stall = 1;
        end else begin
            if_stall = 0;
        end

        if (current_state == IDLE) begin
            mem_stall = 0;
        end else if (ex_mem_re_mem || ex_mem_we_mem) begin
            mem_stall = 1;
        end else begin
            mem_stall = 0;
        end

        if (current_state == IF1 || current_state == WAITFOR1) begin
            imem_ift.r_request_valid = 1;
        end else begin
            imem_ift.r_request_valid = 0;
        end

        if (current_state == IF2 || current_state == WAITFOR2) begin
            imem_ift.r_reply_ready = 1;
        end else begin
            imem_ift.r_reply_ready = 0;
        end

        if(current_state == MEM1) begin
            if (ex_mem_we_mem) begin
                dmem_ift.w_request_valid = 1;
                dmem_ift.r_request_valid = 0;
            end else if (ex_mem_re_mem) begin
                dmem_ift.w_request_valid = 0;
                dmem_ift.r_request_valid = 1;
            end else begin
                dmem_ift.w_request_valid = 0;
                dmem_ift.r_request_valid = 0;
            end
        end else begin
            dmem_ift.r_request_valid = 0;
            dmem_ift.w_request_valid = 0;
        end

        if (current_state == MEM2) begin
            if (ex_mem_we_mem) begin
                dmem_ift.w_reply_ready = 1;
                dmem_ift.r_reply_ready = 0;
            end else if (ex_mem_re_mem) begin
                dmem_ift.w_reply_ready = 0;
                dmem_ift.r_reply_ready = 1;
            end else begin
                dmem_ift.w_reply_ready = 0;
                dmem_ift.r_reply_ready = 0;
            end
        end else begin
            dmem_ift.r_reply_ready = 0;
            dmem_ift.w_reply_ready = 0;
        end
    end
    
    always_comb begin
        imem_ift.r_request_bits.raddr = ro_addr;
        imem_ift.w_request_bits.wdata = 0;
        imem_ift.w_request_bits.waddr = 0;
        imem_ift.w_request_bits.wmask = 0;
        imem_ift.w_request_valid = 0;
        imem_ift.w_reply_ready = 0;
    end

    // Data Trunction
    DataTrunction data_trunc(.read_data(read_data),
                             .memdata_width(ex_mem_memdata_width),
                             .mem_op(ex_mem_mem_op),
                             .alu_res(ex_mem_alu_res),
                             .TruncData(trunc_data)
    );



    // MEM/WB 寄存器传递
    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            mem_wb_inst <= 32'b0;
            mem_wb_alu_res <= 64'b0;
            mem_wb_mem_data <= 64'b0;
            mem_wb_pc <= 64'b0;
            mem_wb_rd <= 5'b0;
            mem_wb_re_mem <= 0;
            mem_wb_we_mem <= 0;
            mem_wb_we_reg <= 1'b0;
            mem_wb_wb_sel <= 2'b0;
            mem_wb_trunc_data <= 64'b0;
            mem_wb_we_csr <= 0;
            mem_wb_csr_ret <= 0;
            mem_wb_csr_val_wb <= 0;
            mem_wb_csr_val <= 0;
            mem_wb_csr_addr <= 0;
            mem_wb_except_exe <= except0;
        end else begin
            if(mem_stall) begin
                mem_wb_inst <= 0;
                mem_wb_alu_res <= 0;
                mem_wb_mem_data <= 0;
                mem_wb_pc <= 0;
                mem_wb_rd <= 0;
                mem_wb_re_mem <= 0;
                mem_wb_we_mem <= 0;
                mem_wb_we_reg <= 0;
                mem_wb_wb_sel <= 0;
                mem_wb_trunc_data <= 0;
                mem_wb_we_csr <= 0;
                mem_wb_csr_ret <= 0;
                mem_wb_csr_val_wb <= 0;
                mem_wb_csr_val <= 0;
                mem_wb_csr_addr <= 0;
                mem_wb_except_exe <= except0;
            end else begin
                mem_wb_alu_res <= ex_mem_alu_res;
                mem_wb_inst <= ex_mem_inst;
                mem_wb_pc <= ex_mem_pc;
                mem_wb_rd <= ex_mem_rd;
                mem_wb_re_mem <= ex_mem_re_mem;
                mem_wb_we_mem <= ex_mem_we_mem;
                mem_wb_we_reg <= ex_mem_we_reg;
                mem_wb_wb_sel <= ex_mem_wb_sel;
                mem_wb_mem_data <= read_data;
                mem_wb_trunc_data <= trunc_data;
                mem_wb_we_csr <= ex_mem_we_csr;
                mem_wb_csr_ret <= ex_mem_csr_ret;
                mem_wb_csr_val_wb <= ex_mem_csr_val_wb;
                mem_wb_csr_val <= ex_mem_csr_val;
                mem_wb_csr_addr <= ex_mem_csr_addr;
                mem_wb_except_exe <= ex_mem_except_exe;
            end
        end
    end

    // ***** WB 阶段 - Write Back *****
    logic [63:0] wb_val;

    always_comb begin
        case (mem_wb_wb_sel)
            WB_SEL_ALU: wb_val = mem_wb_alu_res;
            WB_SEL_MEM: wb_val = mem_wb_trunc_data;
            WB_SEL_PC: wb_val = mem_wb_pc + pc_increment;
            WB_SEL0: wb_val = 64'b0;
        endcase
    end
    
    assign pc_ret = mem_wb_pc + pc_increment;

    assign cosim_valid               = mem_wb_inst != 32'b0;
    assign cosim_core_info.pc        = mem_wb_pc;
    assign cosim_core_info.inst      = {32'b0,mem_wb_inst};   
    // assign cosim_core_info.rs1_id    = {59'b0, rs1};
    // assign cosim_core_info.rs1_data  = read_data_1;
    // assign cosim_core_info.rs2_id    = {59'b0, rs2};
    // assign cosim_core_info.rs2_data  = read_data_2;
    // assign cosim_core_info.alu       = alu_res;
    // assign cosim_core_info.mem_addr  = dmem_ift.r_request_bits.raddr;
    // assign cosim_core_info.mem_we    = {63'b0, dmem_ift.w_request_valid};
    // assign cosim_core_info.mem_wdata = dmem_ift.w_request_bits.wdata;
    // assign cosim_core_info.mem_rdata = dmem_ift.r_reply_bits.rdata;
    assign cosim_core_info.rd_we     = {63'b0, mem_wb_we_reg};
    assign cosim_core_info.rd_id     = {59'b0, mem_wb_rd}; 
    assign cosim_core_info.rd_data   = wb_val;
    // assign cosim_core_info.br_taken  = {63'b0, npc_sel};
    // assign cosim_core_info.npc       = next_pc;

    assign cosim_switch_mode         = switch_mode;

endmodule