module ForwardingMux(
    input logic forward_a,
    input logic forward_b,
    input logic [63:0] read_data_1,
    input logic [63:0] read_data_2,
    input logic [63:0] rs1_sel,
    input logic [63:0] rs2_sel,

    output logic [63:0] real_read_data_1,
    output logic [63:0] real_read_data_2
);

    always_comb begin
        real_read_data_1 = forward_a ? rs1_sel : read_data_1;
        real_read_data_2 = forward_b ? rs2_sel : read_data_2;
    end
endmodule