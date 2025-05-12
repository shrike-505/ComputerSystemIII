`include "core_struct.vh"
module DataTrunction(
    input logic [63:0] read_data,
    input logic [3:0] memdata_width,
    input CorePack::mem_op_enum mem_op,
    input logic [63:0] alu_res,
    output logic [63:0] TruncData
);
    import CorePack::*;
    integer mask_width;
    integer loc;
    always_comb begin
        loc = {29'b0, alu_res[2:0]};
        mask_width = {28'b0, memdata_width};
        mask_width = mask_width * 8;
        for (integer i = 0; i < 64; i = i + 1)begin
            if(i >= mask_width)begin
                case(mem_op)
                    MEM_UW:  TruncData[i] = 0;
                    MEM_UH:  TruncData[i] = 0;
                    MEM_UB:  TruncData[i] = 0;
                    default: TruncData[i] = read_data[loc * 8 + mask_width - 1];
                endcase
            end else begin
                TruncData[i] = read_data[loc * 8 + i];
            end
        end
    end
endmodule