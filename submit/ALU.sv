`include "core_struct.vh"
`include "csr_struct.vh"
module ALU (
    input  CorePack::data_t a,
    input  CorePack::data_t b,
    input  CorePack::alu_op_enum  alu_op,
    output CorePack::data_t res
);

    import CorePack::*;
    
    // fill your code
    wire [31:0] tmp_ADDW;
    wire [31:0] tmp_SUBW;
    wire [31:0] tmp_SLLW;
    wire [31:0] tmp_SRLW;
    wire [31:0] tmp_SRAW;
    assign tmp_ADDW = a[31:0] +  b[31:0];
    assign tmp_SUBW = a[31:0] -  b[31:0];
    assign tmp_SLLW = a[31:0] << b[4:0];
    assign tmp_SRLW = a[31:0] >> b[4:0];
    assign tmp_SRAW = $signed(a[31:0]) >>> b[4:0];
    always_comb begin
        case (alu_op)
            ALU_ADD: res = a + b;
            ALU_SUB: res = a - b;
            ALU_AND: res = a & b;
            ALU_OR: res = a | b;
            ALU_XOR: res = a ^ b;
            ALU_SLT: res = ($signed(a) < $signed(b)) ? 1 : 0;
            ALU_SLTU: res = (a < b) ? 1 : 0;
            ALU_SLL: res = a << b[5:0];
            ALU_SRL: res = a >> b[5:0];
            ALU_SRA: res = $signed(a) >>> b[5:0];
            ALU_ADDW: res = {{32{tmp_ADDW[31]}}, tmp_ADDW[31:0]};
            ALU_SUBW: res = {{32{tmp_SUBW[31]}}, tmp_SUBW[31:0]};
            ALU_SLLW: res = {{32{tmp_SLLW[31]}}, tmp_SLLW[31:0]};
            ALU_SRLW: res = {{32{tmp_SRLW[31]}}, tmp_SRLW[31:0]};
            ALU_SRAW: res = {{32{tmp_SRAW[31]}}, tmp_SRAW[31:0]};
            ALU_DEFAULT: res = 0;
        endcase
    end

endmodule