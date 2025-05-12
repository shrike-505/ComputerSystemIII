`include "core_struct.vh"
`include "csr_struct.vh"
module CSRALU (
    input  CorePack::data_t csr_a,
    input  CorePack::data_t csr_b,
    input  CsrPack::csr_alu_op_enmu  csr_alu_op,
    output CorePack::data_t csr_alu_res
);
    import CsrPack::*;
    always_comb begin
        case(csr_alu_op)
            CSR_ALU_ADD: csr_alu_res = csr_a + csr_b;
            CSR_ALU_OR: csr_alu_res = csr_a | csr_b;
            CSR_ALU_ANDNOT: csr_alu_res = csr_a & (~csr_b);
            default: csr_alu_res = 64'b0;
        endcase
    end
endmodule