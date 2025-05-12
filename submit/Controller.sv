`include "core_struct.vh"
`include "csr_struct.vh"
module Controller (
    input CorePack::inst_t inst,
    output we_reg,
    output we_mem,
    output re_mem,
    output is_b,
    output is_j,
    output CorePack::imm_op_enum immgen_op,
    output CorePack::alu_op_enum alu_op,
    output CorePack::cmp_op_enum cmp_op,
    output CorePack::alu_asel_op_enum alu_asel,
    output CorePack::alu_bsel_op_enum alu_bsel,
    output CorePack::wb_sel_op_enum wb_sel,
    output CorePack::mem_op_enum mem_op,
    output logic [3:0] memdata_width,

    // CSR part
    output CsrPack::csr_reg_ind_t csr_addr_id,
    output logic [1:0] csr_ret,
    output logic [2:0] csr_sel,
    output csr_we_wb,
    output CsrPack::csr_alu_asel_op_enum csr_alu_asel,
    output CsrPack::csr_alu_bsel_op_enum csr_alu_bsel,
    output CsrPack::csr_alu_op_enmu csr_alu_op
);

    import CorePack::*;
    import CsrPack::*;
    funct3_t funct3 = inst[14:12];
    funct7_t funct7 = inst[31:25];
    opcode_t opcode  = inst[6:0];
    csr_reg_ind_t csr_imm = inst[31:20];

    // part for the first stage of decoding, determining the type of instruction
    logic inst_reg = opcode == REG_OPCODE;
    logic inst_regw = opcode == REGW_OPCODE;
    logic inst_load = opcode == LOAD_OPCODE;
    logic inst_lui = opcode == LUI_OPCODE;
    logic inst_store = opcode == STORE_OPCODE;
    logic inst_auipc = opcode == AUIPC_OPCODE;
    logic inst_branch = opcode == BRANCH_OPCODE;
    logic inst_jalr = opcode == JALR_OPCODE;
    logic inst_jal = opcode == JAL_OPCODE;
    logic inst_imm = opcode == IMM_OPCODE;
    logic inst_immw = opcode == IMMW_OPCODE;
    logic inst_csr = opcode == CSR_OPCODE;
    assign csr_we_wb = inst_csr;

    always_comb begin
        we_reg = inst_reg | inst_load | inst_imm | inst_lui | inst_immw | inst_regw | inst_auipc | inst_jalr | inst_jal | inst_csr;
        we_mem = inst_store;
        re_mem = inst_load;
        is_b = inst_branch;
        is_j = inst_jalr | inst_jal;
        immgen_op = inst_imm|inst_load|inst_immw|inst_jalr ? I_IMM : inst_store ? S_IMM : inst_branch ? B_IMM : inst_auipc|inst_lui ? U_IMM : inst_jal ? UJ_IMM : inst_csr ? CSR_IMM : IMM0;
        alu_op = alu_op_enum'(
            ALU_ADD & {4{(inst_load|inst_store|inst_auipc|inst_lui|inst_jalr|inst_jal|inst_branch|inst_csr) | (inst_reg)&(funct3==ADD_FUNCT3)&~inst[30] | (inst_imm)&(funct3==ADD_FUNCT3)}} |
            ALU_SUB & {4{(inst_reg)&inst[30]&(funct3==SUB_FUNCT3)}} |
            ALU_AND & {4{(inst_reg|inst_imm)&(funct3==AND_FUNCT3)}} | 
            ALU_OR & {4{(inst_reg|inst_imm)&(funct3==OR_FUNCT3)}} |
            ALU_XOR & {4{(inst_reg|inst_imm)&(funct3==XOR_FUNCT3)}} |
            ALU_SLT & {4{(inst_reg|inst_imm)&(funct3==SLT_FUNCT3)}} |
            ALU_SLTU &{4{(inst_reg|inst_imm)&(funct3==SLTU_FUNCT3)}} |
            ALU_SLL & {4{(inst_reg|inst_imm)&(funct3==SLL_FUNCT3)}} |
            ALU_SRL & {4{(inst_reg|inst_imm)&(funct3==SRL_FUNCT3)&~inst[30]}} |
            ALU_SRA & {4{(inst_reg|inst_imm)&(funct3==SRA_FUNCT3)&inst[30]}} |
            ALU_ADDW & {4{inst_regw&(funct3==ADDW_FUNCT3)&~inst[30] | inst_immw&(funct3==ADDW_FUNCT3)}} |
            ALU_SUBW & {4{(inst_regw)&(funct3==SUBW_FUNCT3)&inst[30]}} |
            ALU_SLLW & {4{(inst_regw|inst_immw)&(funct3==SLLW_FUNCT3)}} |
            ALU_SRLW & {4{(inst_regw|inst_immw)&(funct3==SRLW_FUNCT3)&~inst[30]}} |
            ALU_SRAW & {4{(inst_regw|inst_immw)&(funct3==SRAW_FUNCT3)&inst[30]}}
        );
        if (inst_reg | inst_regw | inst_load | inst_imm | inst_immw | inst_store | inst_jalr) begin
            alu_asel = ASEL_REG;
        end 
        else begin
            if (inst_auipc | inst_jal | inst_branch) begin 
                alu_asel = ASEL_PC;
            end
            else if (inst_csr) begin
                alu_asel = ASEL_CSR;
            end 
            else begin
                alu_asel = ASEL0;
            end
        end
        if (inst_reg | inst_regw) begin
            alu_bsel = BSEL_REG;
        end 
        else begin 
            if (inst_load | inst_imm | inst_immw | inst_store) begin
                alu_bsel = BSEL_IMM;
            end
            else begin 
                if (inst_branch | inst_jalr | inst_jal | inst_auipc | inst_lui) begin
                    alu_bsel = BSEL_IMM;
                end
                else begin 
                    alu_bsel = BSEL0;
                end
            end
        end
    end

    always_comb begin
        if (inst_branch) begin
            case (funct3)
                BEQ_FUNCT3: cmp_op = CMP_EQ;
                BNE_FUNCT3: cmp_op = CMP_NE;
                BLT_FUNCT3: cmp_op = CMP_LT;
                BGE_FUNCT3: cmp_op = CMP_GE;
                BLTU_FUNCT3: cmp_op = CMP_LTU;
                BGEU_FUNCT3: cmp_op = CMP_GEU;
                default: cmp_op = CMP_NO;
            endcase
        end 
        else begin
            cmp_op = CMP_NO;
        end
    end

    always_comb begin
        if (inst_load|inst_store) begin
            case (funct3)
                3'b000:begin
                    mem_op = MEM_B;
                    memdata_width = 1;
                end
                3'b001:begin
                    mem_op = MEM_H;
                    memdata_width = 2;
                end
                3'b010:begin
                    mem_op = MEM_W;
                    memdata_width = 4;
                end
                3'b011:begin
                    mem_op = MEM_D;
                    memdata_width = 8;
                end
                3'b100:begin
                    mem_op = MEM_UB;
                    memdata_width = 1;
                end
                3'b101:begin
                    mem_op = MEM_UH;
                    memdata_width = 2;
                end
                3'b110:begin
                    mem_op = MEM_UW;
                    memdata_width = 4;
                end
                default:begin
                    mem_op = MEM_NO;
                    memdata_width = 0;
                end
            endcase
        end
        else begin
            mem_op = MEM_NO;
            memdata_width = 0;
        end
    end

    always_comb begin
        if (inst_load) begin
            wb_sel = WB_SEL_MEM;
        end 
        else begin 
            if(inst_csr|inst_imm|inst_immw|inst_reg|inst_regw|inst_auipc|inst_lui) begin
                wb_sel = WB_SEL_ALU;
            end 
            else begin 
                if(inst_jalr|inst_jal) begin
                    wb_sel = WB_SEL_PC;
                end
                else begin
                    wb_sel = WB_SEL0; 
                end
            end
        end
    end

    always_comb begin
        // CSR part
        if(inst_csr) begin
            csr_addr_id = csr_imm;
            case (funct3)
                CSRRW_FUNCT3: begin
                    csr_sel = 1;
                    csr_alu_asel = ASEL_CSR0;
                    csr_alu_bsel = BSEL_GPREG;
                    csr_alu_op = CSR_ALU_ADD;
                end
                CSRRS_FUNCT3: begin
                    csr_sel = 2;
                    csr_alu_asel = ASEL_CSRREG;
                    csr_alu_bsel = BSEL_GPREG;
                    csr_alu_op = CSR_ALU_OR;
                end
                CSRRC_FUNCT3: begin
                    csr_sel = 3;
                    csr_alu_asel = ASEL_CSRREG;
                    csr_alu_bsel = BSEL_GPREG;
                    csr_alu_op = CSR_ALU_ANDNOT;
                end
                CSRRWI_FUNCT3: begin
                    csr_sel = 4;
                    csr_alu_asel = ASEL_CSR0;
                    csr_alu_bsel = BSEL_CSRIMM;
                    csr_alu_op = CSR_ALU_ADD;
                end
                CSRRSI_FUNCT3: begin
                    csr_sel = 5;
                    csr_alu_asel = ASEL_CSRREG;
                    csr_alu_bsel = BSEL_CSRIMM;
                    csr_alu_op = CSR_ALU_OR;
                end
                CSRRCI_FUNCT3: begin
                    csr_sel = 6;
                    csr_alu_asel = ASEL_CSRREG;
                    csr_alu_bsel = BSEL_CSRIMM;
                    csr_alu_op = CSR_ALU_ANDNOT;
                end
                default: begin
                    csr_sel = 0;
                    csr_alu_asel = ASEL_CSR0;
                    csr_alu_bsel = BSEL_CSR0;
                    csr_alu_op = CSR_ALU_ADD;
                end
            endcase
        end else begin
            csr_ret = 2'b00;
            csr_sel = 3'b000;
            csr_addr_id = 0;
            csr_alu_asel = ASEL_CSR0;
            csr_alu_bsel = BSEL_CSR0;
            csr_alu_op = CSR_ALU_ADD;
        end
        if(inst == MRET) begin
            csr_ret = 2'b10;
        end else if(inst == SRET) begin
            csr_ret = 2'b01;
        end else begin
            csr_ret = 2'b00;
        end
    end

endmodule