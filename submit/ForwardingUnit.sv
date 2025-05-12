`include "core_struct.vh"
module ForwardingUnit (
    input logic [4:0] rs1,       // 当前指令的源寄存器1
    input logic [4:0] rs2,       // 当前指令的源寄存器2
    input logic [4:0] id_ex_rd,     // ID阶段的目的寄存器
    input logic [4:0] ex_mem_rd,     // EX阶段的目的寄存器
    input logic [4:0] mem_wb_rd,    // MEM阶段的目的寄存器
    input logic id_ex_we_reg,       // ID阶段是否写寄存器
    input logic ex_mem_we_reg,       // EX阶段是否写寄存器
    input logic mem_wb_we_reg,      // MEM阶段是否写寄存器
    input logic [63:0] alu_res,
    input logic [63:0] ex_mem_alu_res,
    input logic [63:0] wb_val,

    input logic mem_wb_re_mem,
    input logic [63:0] mem_wb_trunc_data,

    output logic [63:0] rs1_sel,
    output logic [63:0] rs2_sel,
    output logic forward_a,
    output logic forward_b
);
    always_comb begin
        // 默认不前递
        rs1_sel=0;
        rs2_sel=0;
        forward_a=0;
        forward_b=0;

        // 检查 id_exe_rs1 的前递
        if (id_ex_we_reg && id_ex_rd == rs1 && rs1 != 0) begin
            rs1_sel = alu_res;
            forward_a = 1;
        end else if(ex_mem_we_reg && ex_mem_rd == rs1 && rs1 != 0) begin
            rs1_sel = ex_mem_alu_res;
            forward_a = 1;
        end else if(mem_wb_we_reg && mem_wb_rd == rs1 && rs1 != 0) begin
            rs1_sel = wb_val;
            forward_a = 1;
        end else if(mem_wb_re_mem && mem_wb_rd == rs1 && rs1 != 0) begin
            rs1_sel = mem_wb_trunc_data;
            forward_a = 1;
        end

        // 检查 id_exe_rs2 的前递
        if (id_ex_we_reg && id_ex_rd == rs2 && rs2 != 0) begin
            rs2_sel = alu_res;
            forward_b = 1;
        end else if(ex_mem_we_reg && ex_mem_rd == rs2 && rs2 != 0) begin
            rs2_sel = ex_mem_alu_res;
            forward_b = 1;
        end else if(mem_wb_we_reg && mem_wb_rd == rs2 && rs2 != 0) begin
            rs2_sel = wb_val;
            forward_b = 1;
        end else if(mem_wb_re_mem && mem_wb_rd == rs2 && rs2 != 0) begin
            rs2_sel = mem_wb_trunc_data;
            forward_b = 1;
        end
        
    end
endmodule