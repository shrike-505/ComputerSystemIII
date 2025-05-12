module DataPackage( // with mask
    input logic [63:0] alu_res,
    input logic [3:0] memdata_width,
    input logic [63:0] read_data,
    output logic [63:0] wdata_data_packaged,
    output logic [7:0] wdata_mask_packaged
);
    integer loc = {29'b0, alu_res[2:0]};
    integer mask_width;

    always_comb begin
        mask_width = {28'b0, memdata_width} * 8;
        for (integer i = 0; i < 8; i = i + 1) begin
            if (i < loc + mask_width / 8 && i >= loc) begin
                wdata_mask_packaged[i] = 1;
            end else begin
                wdata_mask_packaged[i] = 0;
            end
        end
        for (integer i = 0; i < 64; i = i + 1) begin
            if (i < loc * 8) begin
                wdata_data_packaged[i] = 0;
            end else begin
                wdata_data_packaged[i] = read_data[i - loc * 8];
            end
        end
    end

endmodule