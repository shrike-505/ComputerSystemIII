`include "core_struct.vh"
module ImmGen(
    input [31:0] inst,
    input CorePack::imm_op_enum immgen_op,
    output logic [63:0] Imm
);
    import CorePack::*;
    always_comb begin
        case (immgen_op)
            IMM0: Imm = 64'b0;
            I_IMM: Imm = {{52{inst[31]}}, inst[31:20]};
            S_IMM: Imm = {{52{inst[31]}}, inst[31:25], inst[11:7]};
            B_IMM: Imm = {{51{inst[31]}}, inst[31], inst[7], inst[30:25], inst[11:8], 1'b0};
            U_IMM: Imm = {{32{inst[31]}}, inst[31:12], 12'b0};
            UJ_IMM: Imm = {{43{inst[31]}}, inst[31], inst[19:12], inst[20], inst[30:21], 1'b0};
            CSR_IMM: Imm = {{59'b0}, inst[19:15]};
            default: Imm = 64'b0;
        endcase
    end
endmodule