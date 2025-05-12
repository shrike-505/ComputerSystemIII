`include "core_struct.vh"
module HazardDetection (
    input  logic [4:0] rs1,
    input logic [4:0] rs2,
    input  logic [4:0] id_ex_rd,         // EXE阶段的目标寄存器
    input  logic [4:0] ex_mem_rd,        // MEM阶段的目标寄存器
    input  logic id_ex_re_mem,
    input  logic ex_mem_re_mem,
    output logic stall                   // 输出 Stall 信号
);

    always_comb begin
        stall = 0;  // 默认不暂停

        // **EXE 阶段数据冒险**：ID 阶段需要读取的寄存器与 EXE 阶段的写回寄存器冲突。
        if (id_ex_re_mem && (rs1 == id_ex_rd || rs2 == id_ex_rd) && (id_ex_rd != 0)) begin
            stall = 1;  // 插入暂停
        end

        // **MEM 阶段数据冒险**：ID 阶段需要读取的寄存器与 MEM 阶段的写回寄存器冲突。
        else if (ex_mem_re_mem && ((rs1 == ex_mem_rd) || (rs2 == ex_mem_rd)) && (ex_mem_rd != 0)) begin
            stall = 1;  // 插入暂停
        end
    end
endmodule