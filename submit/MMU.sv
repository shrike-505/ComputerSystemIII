`include "core_struct.vh"
`include "csr_struct.vh"
`include "mem_ift.vh"

module MMU(
    input clk,
    input rst,
    input CorePack::data_t satp,
    input [1:0] priv,
    input switch_mode,
    input CorePack::data_t pc_if, // To be used
    input CorePack::data_t pc_mem, // To be used

    output CsrPack::ExceptPack except_mmu, // TBD

    Mem_ift.Slave core_imem_ift,
    Mem_ift.Slave core_dmem_ift,

    Mem_ift.Master mem_imem_ift,
    Mem_ift.Master mem_dmem_ift
);
    import CorePack::*;

    typedef enum {
        IDLE,
        PTWALK1_1,
        PTWALK1_2,
        PTWALK2_1,
        PTWALK2_2,
        PTWALK3_1,
        PTWALK3_2,
        GET_DATA,
        DIRECT
    } mmu_state_t;

    localparam PTE_V = 0;
    localparam PTE_R = 1;
    localparam PTE_W = 2;
    localparam PTE_X = 3;
    localparam PTE_U = 4;
    localparam PTE_G = 5;
    localparam PTE_A = 6;
    localparam PTE_D = 7;
    localparam PTE_RSW_LOW = 8;
    localparam PTE_RSW_HIGH = 9;
    localparam PTE_PPN_LOW = 10;
    localparam PTE_PPN_HIGH = 53;

    // 从两种 mem 共三种操作（读 imem，读 dmem，写 dmem）取出的三层页表的 pte
    data_t imem_pte_level_1, r_dmem_pte_level_1, w_dmem_pte_level_1;
    data_t imem_pte_level_2, r_dmem_pte_level_2, w_dmem_pte_level_2;
    data_t imem_pte_level_3, r_dmem_pte_level_3, w_dmem_pte_level_3;

    // satp 的 ppn 字段
    logic [43:0] satp_ppn;
    assign satp_ppn = satp[43:0];

    // 000: direct
    // 001: level 1
    // 010: level 2
    // 100: level 3
    logic [2:0] pte_level;

    // read imem 的 addr 的 vpn
    logic [8:0] ivpn [2:0];

    // read dmem 的 addr 的 vpn
    logic [8:0] dvpn_r [2:0];

    // write dmem 的 addr 的 vpn
    logic [8:0] dvpn_w [2:0];



    // 用于存储某个周期时 mem 的 request 是否握手成功
    // 在 PTWALKX_1 中被非阻塞赋值存储，在下个状态（PTWALKX_2）中根据 request 握手的情况对对应 reply_ready 赋 1
    logic request_handshake_r_imem;
    logic request_handshake_r_dmem;
    logic request_handshake_w_dmem;

    // offset[2]: read imem 的 addr 的 offset
    // offset[1]: read dmem 的 addr 的 offset
    // offset[0]: write dmem 的 addr 的 offset
    logic [11:0] offset [2:0];
    always_comb begin
        // 来自 core 的 va，提取出各类 vpn 和 offset
        ivpn[2]   = core_imem_ift.r_request_bits.raddr[38:30];
        ivpn[1]   = core_imem_ift.r_request_bits.raddr[29:21];
        ivpn[0]   = core_imem_ift.r_request_bits.raddr[20:12];
        offset[2] = core_imem_ift.r_request_bits.raddr[11:0];
        dvpn_r[2] = core_dmem_ift.r_request_bits.raddr[38:30];
        dvpn_r[1] = core_dmem_ift.r_request_bits.raddr[29:21];
        dvpn_r[0] = core_dmem_ift.r_request_bits.raddr[20:12];
        offset[1] = core_dmem_ift.r_request_bits.raddr[11:0];
        dvpn_w[2] = core_dmem_ift.w_request_bits.waddr[38:30];
        dvpn_w[1] = core_dmem_ift.w_request_bits.waddr[29:21];
        dvpn_w[0] = core_dmem_ift.w_request_bits.waddr[20:12];
        offset[0] = core_dmem_ift.w_request_bits.waddr[11:0];
    end


    mmu_state_t current_state, next_state;    
    // 连接内存
    always_comb begin
        mem_imem_ift.r_request_valid = 0;
        mem_imem_ift.r_reply_ready = 0;
        mem_dmem_ift.r_request_valid = 0;
        mem_dmem_ift.r_reply_ready = 0;
        mem_dmem_ift.w_request_valid = 0;
        mem_dmem_ift.w_reply_ready = 0;

        core_imem_ift.r_request_ready = 0;
        core_imem_ift.r_reply_valid = 0;
        core_dmem_ift.r_request_ready = 0;
        core_dmem_ift.r_reply_valid = 0;
        core_dmem_ift.w_request_ready = 0;
        core_dmem_ift.w_reply_valid = 0;


        case (current_state)
            IDLE: begin

            end
            DIRECT: begin
                // core_imem_ift.w_request_ready = mem_imem_ift.w_request_ready;
                // mem_imem_ift.w_request_valid = core_imem_ift.w_request_valid;
                core_imem_ift.r_reply_bits.rdata = mem_imem_ift.r_reply_bits.rdata;
                // core_imem_ift.w_reply_valid = mem_imem_ift.w_reply_valid;
                // mem_imem_ift.w_reply_ready = core_imem_ift.w_reply_ready;
                core_imem_ift.r_request_ready = mem_imem_ift.r_request_ready;
                mem_imem_ift.r_request_valid = core_imem_ift.r_request_valid;
                core_imem_ift.r_reply_valid = mem_imem_ift.r_reply_valid;
                mem_imem_ift.r_reply_ready = core_imem_ift.r_reply_ready;
                mem_imem_ift.r_request_bits.raddr= core_imem_ift.r_request_bits.raddr;
                // mem_imem_ift.w_request_bits.waddr = core_imem_ift.w_request_bits.waddr;
                // mem_imem_ift.w_request_bits.wdata = core_imem_ift.w_request_bits.wdata;
                // mem_imem_ift.w_request_bits.wmask = core_imem_ift.w_request_bits.wmask;
                core_dmem_ift.w_request_ready = mem_dmem_ift.w_request_ready;
                mem_dmem_ift.w_request_valid = core_dmem_ift.w_request_valid;
                core_dmem_ift.r_reply_bits.rdata = mem_dmem_ift.r_reply_bits.rdata;
                core_dmem_ift.w_reply_valid = mem_dmem_ift.w_reply_valid;
                mem_dmem_ift.w_reply_ready = core_dmem_ift.w_reply_ready;
                core_dmem_ift.r_request_ready = mem_dmem_ift.r_request_ready;
                mem_dmem_ift.r_request_valid = core_dmem_ift.r_request_valid;
                core_dmem_ift.r_reply_valid = mem_dmem_ift.r_reply_valid;
                mem_dmem_ift.r_reply_ready = core_dmem_ift.r_reply_ready;
                mem_dmem_ift.r_request_bits.raddr = core_dmem_ift.r_request_bits.raddr;
                mem_dmem_ift.w_request_bits.waddr = core_dmem_ift.w_request_bits.waddr;
                mem_dmem_ift.w_request_bits.wdata = core_dmem_ift.w_request_bits.wdata;
                mem_dmem_ift.w_request_bits.wmask = core_dmem_ift.w_request_bits.wmask;                        
            end
            PTWALK1_1: begin
                mem_imem_ift.r_request_valid = core_imem_ift.r_request_valid;
                mem_dmem_ift.r_request_valid = core_dmem_ift.r_request_valid || core_dmem_ift.w_request_valid;

                //  目标页表项（此处即一级页表中的页表项）地址 = satp << 12（页表基地址） + vpn[2] << 3
                if (core_imem_ift.r_request_valid) begin
                    mem_imem_ift.r_request_bits.raddr = {8'b0, satp_ppn, ivpn[2], 3'b0};
                end
                if (core_dmem_ift.r_request_valid) begin
                    mem_dmem_ift.r_request_bits.raddr = {8'b0, satp_ppn, dvpn_r[2], 3'b0}; 
                end
                if (core_dmem_ift.w_request_valid) begin
                    mem_dmem_ift.r_request_bits.raddr = {8'b0, satp_ppn, dvpn_w[2], 3'b0}; 
                end
            end

            PTWALK1_2: begin
                // 对应 request 在 PTWALK1_1 的握手成功时，才将 reply 置为 ready
                // PTWALK2/3 同理
                if (request_handshake_r_imem) begin
                    mem_imem_ift.r_reply_ready = 1;
                end
                if (request_handshake_r_dmem) begin
                    mem_dmem_ift.r_reply_ready = 1;
                end
                if (request_handshake_w_dmem) begin
                    mem_dmem_ift.w_reply_ready = 1;
                end
            end

            PTWALK2_1: begin
                mem_imem_ift.r_request_valid = core_imem_ift.r_request_valid;
                mem_dmem_ift.r_request_valid = core_dmem_ift.r_request_valid || core_dmem_ift.w_request_valid;
                if (core_imem_ift.r_request_valid) begin
                    mem_imem_ift.r_request_bits.raddr = {8'b0, imem_pte_level_1[PTE_PPN_HIGH:PTE_PPN_LOW], ivpn[1], 3'b0};
                end
                if (core_dmem_ift.r_request_valid) begin
                    mem_dmem_ift.r_request_bits.raddr = {8'b0, r_dmem_pte_level_1[PTE_PPN_HIGH:PTE_PPN_LOW], dvpn_r[1], 3'b0};
                end
                if (core_dmem_ift.w_request_valid) begin
                    mem_dmem_ift.r_request_bits.raddr = {8'b0, r_dmem_pte_level_1[PTE_PPN_HIGH:PTE_PPN_LOW], dvpn_w[1], 3'b0};
                end
            end

            PTWALK2_2: begin
                if (request_handshake_r_imem) begin
                    mem_imem_ift.r_reply_ready = 1;
                end
                if (request_handshake_r_dmem) begin
                    mem_dmem_ift.r_reply_ready = 1;
                end
                if (request_handshake_w_dmem) begin
                    mem_dmem_ift.w_reply_ready = 1;
                end
            end

            PTWALK3_1: begin
                mem_imem_ift.r_request_valid = core_imem_ift.r_request_valid;
                mem_dmem_ift.r_request_valid = core_dmem_ift.r_request_valid || core_dmem_ift.w_request_valid;
                if (core_imem_ift.r_request_valid) begin
                    mem_imem_ift.r_request_bits.raddr = {8'b0, imem_pte_level_2[PTE_PPN_HIGH:PTE_PPN_LOW], ivpn[0], 3'b0};
                end
                if (core_dmem_ift.r_request_valid) begin
                    mem_dmem_ift.r_request_bits.raddr = {8'b0, r_dmem_pte_level_2[PTE_PPN_HIGH:PTE_PPN_LOW], dvpn_r[0],3'b0};
                end
                if (core_dmem_ift.w_request_valid) begin
                    mem_dmem_ift.r_request_bits.raddr = {8'b0, r_dmem_pte_level_2[PTE_PPN_HIGH:PTE_PPN_LOW], dvpn_w[0], 3'b0};
                end
            end

            PTWALK3_2: begin
                if (request_handshake_r_imem) begin
                    mem_imem_ift.r_reply_ready = 1;
                end
                if (request_handshake_r_dmem) begin
                    mem_dmem_ift.r_reply_ready = 1;
                end
                if (request_handshake_w_dmem) begin
                    mem_dmem_ift.w_reply_ready = 1;
                end
            end


            GET_DATA: begin

                // 先连接握手信号
                mem_imem_ift.r_request_valid = core_imem_ift.r_request_valid;
                mem_imem_ift.r_reply_ready = core_imem_ift.r_reply_ready;
                mem_dmem_ift.r_request_valid = core_dmem_ift.r_request_valid;
                mem_dmem_ift.r_reply_ready = core_dmem_ift.r_reply_ready;
                mem_dmem_ift.w_request_valid = core_dmem_ift.w_request_valid;
                mem_dmem_ift.w_reply_ready = core_dmem_ift.w_reply_ready;

                core_imem_ift.r_request_ready = mem_imem_ift.r_request_ready;
                core_imem_ift.r_reply_valid = mem_imem_ift.r_reply_valid;
                core_dmem_ift.r_request_ready = mem_dmem_ift.r_request_ready;
                core_dmem_ift.r_reply_valid = mem_dmem_ift.r_reply_valid;
                core_dmem_ift.w_request_ready = mem_dmem_ift.w_request_ready;
                core_dmem_ift.w_reply_valid = mem_dmem_ift.w_reply_valid;

                if (pte_level == 3'b001) begin
                    // 一级页表
                    // 根据一级页表寻址得到各种真正的物理地址
                    mem_imem_ift.r_request_bits.raddr = {8'b0, imem_pte_level_1[PTE_PPN_HIGH:28], ivpn[1], ivpn[0], offset[2]};
                    mem_dmem_ift.r_request_bits.raddr = {8'b0, r_dmem_pte_level_1[PTE_PPN_HIGH:28], dvpn_r[1], dvpn_r[0], offset[1]};
                    mem_dmem_ift.w_request_bits.waddr = {8'b0, r_dmem_pte_level_1[PTE_PPN_HIGH:28], dvpn_w[1], dvpn_w[0], offset[0]};
                end else if (pte_level == 3'b100) begin
                    //三级页表
                    mem_imem_ift.r_request_bits.raddr = {8'b0, imem_pte_level_3[PTE_PPN_HIGH:PTE_PPN_LOW], offset[2]};
                    mem_dmem_ift.r_request_bits.raddr = {8'b0, r_dmem_pte_level_3[PTE_PPN_HIGH:PTE_PPN_LOW], offset[1]};
                    mem_dmem_ift.w_request_bits.waddr = {8'b0, r_dmem_pte_level_3[PTE_PPN_HIGH:PTE_PPN_LOW], offset[0]};
                end
                
                // 将读到的数据传回 core / 要写入的数据写进 mem

                // if (mem_imem_ift.r_reply_ready && mem_imem_ift.r_reply_valid) begin
                //     core_imem_ift.r_reply_bits.rdata = mem_imem_ift.r_reply_bits.rdata;
                // end
                // if (mem_dmem_ift.r_reply_ready && mem_dmem_ift.r_reply_valid) begin
                //     core_dmem_ift.r_reply_bits.rdata = mem_dmem_ift.r_reply_bits.rdata;
                // end

                // if (core_dmem_ift.w_request_valid && core_dmem_ift.w_request_ready) begin
                //     mem_dmem_ift.w_request_bits.wdata = core_dmem_ift.w_request_bits.wdata;
                //     mem_dmem_ift.w_request_bits.wmask = core_dmem_ift.w_request_bits.wmask;
                // end
                core_imem_ift.r_reply_bits.rdata = mem_imem_ift.r_reply_bits.rdata;
                core_dmem_ift.r_reply_bits.rdata = mem_dmem_ift.r_reply_bits.rdata;
                mem_dmem_ift.w_request_bits.wdata = core_dmem_ift.w_request_bits.wdata;
                mem_dmem_ift.w_request_bits.wmask = core_dmem_ift.w_request_bits.wmask;      
            end
        endcase
    end


    // 更新状态
    always_ff @(posedge clk or negedge rst) begin
        if (rst) begin
            current_state <= IDLE;
        end else begin
            current_state <= next_state;
        end
    end

    // 处理需要保存到下一个状态的信号
    always_ff @(posedge clk or negedge rst) begin
        case (current_state)
            IDLE: begin
                pte_level <= 3'b000;
            end
            DIRECT: begin
                pte_level <= 3'b000;
            end

            PTWALK1_1: begin
                pte_level <= 3'b001;
                request_handshake_r_imem <= mem_imem_ift.r_request_valid && mem_imem_ift.r_request_ready;
                request_handshake_r_dmem <= mem_dmem_ift.r_request_valid && mem_dmem_ift.r_request_ready;
                request_handshake_w_dmem <= mem_dmem_ift.w_request_valid && mem_dmem_ift.w_request_ready;
            end

            PTWALK1_2: begin
                pte_level <= 3'b001;

                // 根据 PTWALK1_1的 raddr 读取对应 pte
                if (mem_imem_ift.r_reply_valid && mem_imem_ift.r_reply_ready) begin
                    imem_pte_level_1 <= mem_imem_ift.r_reply_bits.rdata;
                end

                if (mem_dmem_ift.r_reply_valid && mem_dmem_ift.r_reply_ready) begin
                    r_dmem_pte_level_1 <= mem_dmem_ift.r_reply_bits.rdata;
                end

                // w_dmem_pte_level_x may be useless...?
                if (mem_dmem_ift.w_reply_valid && mem_dmem_ift.w_reply_ready) begin
                    w_dmem_pte_level_1 <= mem_dmem_ift.r_reply_bits.rdata;
                end
            end
            PTWALK2_1: begin
                pte_level <= 3'b010;
                request_handshake_r_imem <= mem_imem_ift.r_request_valid && mem_imem_ift.r_request_ready;
                request_handshake_r_dmem <= mem_dmem_ift.r_request_valid && mem_dmem_ift.r_request_ready;
                request_handshake_w_dmem <= mem_dmem_ift.w_request_valid && mem_dmem_ift.w_request_ready;
            end

            PTWALK2_2: begin
                pte_level <= 3'b010;
                if (mem_imem_ift.r_reply_valid && mem_imem_ift.r_reply_ready) begin
                    imem_pte_level_2 <= mem_imem_ift.r_reply_bits.rdata;
                end

                if (mem_dmem_ift.r_reply_valid && mem_dmem_ift.r_reply_ready) begin
                    r_dmem_pte_level_2 <= mem_dmem_ift.r_reply_bits.rdata;
                end

                if (mem_dmem_ift.w_reply_valid && mem_dmem_ift.w_reply_ready) begin
                    w_dmem_pte_level_2 <= mem_dmem_ift.r_reply_bits.rdata;
                end
            end

            PTWALK3_1:begin

                pte_level <= 3'b100;
                request_handshake_r_imem <= mem_imem_ift.r_request_valid && mem_imem_ift.r_request_ready;
                request_handshake_r_dmem <= mem_dmem_ift.r_request_valid && mem_dmem_ift.r_request_ready;
                request_handshake_w_dmem <= mem_dmem_ift.w_request_valid && mem_dmem_ift.w_request_ready;

            end
            PTWALK3_2: begin
                pte_level <= 3'b100;
                if (mem_imem_ift.r_reply_valid && mem_imem_ift.r_reply_ready) begin
                    imem_pte_level_3 <= mem_imem_ift.r_reply_bits.rdata;
                end

                if (mem_dmem_ift.r_reply_valid && mem_dmem_ift.r_reply_ready) begin
                    r_dmem_pte_level_3 <= mem_dmem_ift.r_reply_bits.rdata;
                end

                if (mem_dmem_ift.w_reply_valid && mem_dmem_ift.w_reply_ready) begin
                    w_dmem_pte_level_3 <= mem_dmem_ift.r_reply_bits.rdata;
                end
            end
        endcase
    end

    // 判断当前页表项是否为叶子节点，由于判断要发生在 PTWALKX_2 中，而 xmem_pte_level_x 由于非阻塞赋值，其值从下一个状态（周期）才会改变，于是用读到的 rdata 来判断
    logic is_leaf_pte = (mem_imem_ift.r_reply_bits.rdata[PTE_X] == 1'b1 && mem_imem_ift.r_reply_bits.rdata[PTE_R] == 1'b1 && mem_imem_ift.r_reply_bits.rdata[PTE_W] == 1'b1) || (mem_dmem_ift.r_reply_bits.rdata[PTE_X] == 1'b1 && mem_dmem_ift.r_reply_bits.rdata[PTE_R] == 1'b1 && mem_dmem_ift.r_reply_bits.rdata[PTE_W] == 1'b1);

    always_comb begin
        next_state = current_state;
        case (current_state)
            IDLE: begin
                if(satp[63:60] == 8 && priv != 2'b11) begin
                    // Sv39
                    next_state = PTWALK1_1;
                end else begin
                    next_state = DIRECT;
                end
            end
            DIRECT: begin
                if (mem_imem_ift.r_reply_valid && mem_imem_ift.r_reply_ready || mem_dmem_ift.r_reply_valid && mem_dmem_ift.r_reply_ready || mem_dmem_ift.w_reply_valid && mem_dmem_ift.w_reply_ready) begin
                    next_state = IDLE;
                end
            end

            PTWALK1_1: begin
                if (mem_imem_ift.r_request_valid && mem_imem_ift.r_request_ready || mem_dmem_ift.r_request_valid && mem_dmem_ift.r_request_ready || mem_dmem_ift.w_request_valid && mem_dmem_ift.w_request_ready) begin
                    next_state = PTWALK1_2;
                end
            end
            PTWALK1_2: begin
                if (switch_mode) begin
                    next_state = GET_DATA;
                end else begin
                    if (mem_imem_ift.r_reply_valid && mem_imem_ift.r_reply_ready || mem_dmem_ift.r_reply_valid && mem_dmem_ift.r_reply_ready || mem_dmem_ift.w_reply_valid && mem_dmem_ift.w_reply_ready) begin
                        if (is_leaf_pte) begin
                            next_state = GET_DATA;
                        end else begin
                            next_state = PTWALK2_1;
                        end
                    end
                end
            end

            PTWALK2_1: begin
                if (mem_imem_ift.r_request_valid && mem_imem_ift.r_request_ready || mem_dmem_ift.r_request_valid && mem_dmem_ift.r_request_ready || mem_dmem_ift.w_request_valid && mem_dmem_ift.w_request_ready) begin
                    next_state = PTWALK2_2;
                end

            end
            PTWALK2_2: begin
                if (mem_imem_ift.r_reply_valid && mem_imem_ift.r_reply_ready || mem_dmem_ift.r_reply_valid && mem_dmem_ift.r_reply_ready || mem_dmem_ift.w_reply_valid && mem_dmem_ift.w_reply_ready) begin
                    // 我不确定这里需不需要判断叶子pte，因为不存在 PTWALK2_2 直接跳到 GET_DATA 的情况（？）

                    // if ((mem_imem_ift.r_reply_bits.rdata[PTE_X] == 1'b1 && mem_imem_ift.r_reply_bits.rdata[PTE_R] == 1'b1 && mem_imem_ift.r_reply_bits.rdata[PTE_W] == 1'b1) || (mem_dmem_ift.r_reply_bits.rdata[PTE_X] == 1'b1 && mem_dmem_ift.r_reply_bits.rdata[PTE_R] == 1'b1 && mem_dmem_ift.r_reply_bits.rdata[PTE_W] == 1'b1)) begin
                    //     next_state = GET_DATA;
                    // end else begin
                    next_state = PTWALK3_1;
                    // end
                end
            end

            PTWALK3_1: begin
                if (mem_imem_ift.r_request_valid && mem_imem_ift.r_request_ready || mem_dmem_ift.r_request_valid && mem_dmem_ift.r_request_ready || mem_dmem_ift.w_request_valid && mem_dmem_ift.w_request_ready) begin
                    next_state = PTWALK3_2;
                end
            end
            PTWALK3_2: begin

                // 这里应该也不需要判断？因为已经是三级页表，取出的 pte 一定是叶子节点（？）
                if (mem_imem_ift.r_reply_valid && mem_imem_ift.r_reply_ready || mem_dmem_ift.r_reply_valid && mem_dmem_ift.r_reply_ready || mem_dmem_ift.w_reply_valid && mem_dmem_ift.w_reply_ready) begin
                    next_state = GET_DATA;
                end
            end

            GET_DATA: begin

                // core 的 reply 握手完成代表一整套流程结束，回到 IDLE
                if (core_imem_ift.r_reply_valid && core_imem_ift.r_reply_ready || core_dmem_ift.r_reply_valid && core_dmem_ift.r_reply_ready || core_dmem_ift.w_reply_valid && core_dmem_ift.w_reply_ready) begin
                    next_state = IDLE;
                end
            end
        endcase
    end
                
endmodule