# -*- coding: utf-8 -*-
# 补第 15 章作业 2~5 的可执行结果（纯计算/模拟）

def tlb():
    print("=== 任务2：不同页表方案的翻译访存次数（TLB 未命中） ===")
    print("TLB 命中  : 0 次额外访存")
    print("单级页表  : 1 次（页表）")
    print("二级页表  : 2 次（页目录 + 页表）")
    print("四级页表  : 4 次（PGD + PUD + PMD + PT）")
    print("结论：命中率 99% 时，平均额外访存 = 0.01 * 层级数 ≈ 0，延迟被 TLB 掩盖\n")

def inverted(fracs=(0.01, 0.1, 0.5, 0.9)):
    print("=== 任务3：倒排页表哈希冲突（物理 128 页框，哈希表 128 槽） ===")
    import random
    nframes = 128
    for frac in fracs:
        n_active = int(nframes * frac)          # 活跃映射数 = 占用页框数
        slots = [[] for _ in range(nframes)]    # 链地址法
        collisions = 0
        for i in range(n_active):
            h = (hash((i % 7, i)) & 0x7fffffff) % nframes
            if slots[h]:
                collisions += 1                 # 命中已占用槽 = 冲突
            slots[h].append(i)
        print(f"  活跃映射 {n_active:3d}（占用 {frac:.0%}）：冲突 {collisions:3d} 次，"
              f"平均链长 {n_active/nframes:.2f}")
    print("结论：占用越满、映射越多，冲突越多，查找平均链长越长\n")

def segmented():
    print("=== 任务4：段页式 vs 多级页表，页表占用（32 位空间，4KB 页） ===")
    # 32 位地址空间 = 2^20 个 4KB 页；单级页表 4MB
    single = (1 << 20) * 4
    # 二级：页目录 1 页 + 每段一张页表（每张覆盖 4MB = 1024 页）
    sparse_segs = 3   # 代码/堆/栈 三段，段间大片空洞
    dense_segs  = 12
    two_sparse = 4096 + sparse_segs * 4096
    two_dense  = 4096 + dense_segs  * 4096
    def mb(b): return f"{b/1024/1024:.2f}MB"
    print(f"  单级线性页表           : {mb(single)}（2^20 个 PTE，整表建全）")
    print(f"  二级页表(稀疏 3 段)    : {mb(two_sparse)}")
    print(f"  二级页表(连续 12 段)   : {mb(two_dense)}")
    print(f"结论：稀疏(3 段)比单级省 {100*(1-two_sparse/single):.0f}%，"
          f"连续(12 段)只省 {100*(1-two_dense/single):.0f}%——越稀疏段页式越划算\n")

def large_pages():
    print("=== 任务5：大页对层级和 TLB 覆盖的影响（32 位空间） ===")
    for psize, name in [(4096,'4KB'), (2*1024*1024,'2MB'), (1024*1024*1024,'1GB')]:
        offset_bits = psize.bit_length()-1
        vpn_bits = 32 - offset_bits
        # 大页能少一层：一页覆盖的内存
        covered_per_tlb = psize * 1024   # 假设 TLB 有 1024 项
        print(f"  {name}: offset {offset_bits} 位, VPN {vpn_bits} 位, "
              f"1024 个 TLB 条目共覆盖 {covered_per_tlb/1024/1024:.0f}MB")
    print("结论：页越大，VPN 位数越少（层级少一级）、同样 TLB 覆盖内存越大；但内碎片越大\n")

if __name__ == '__main__':
    tlb(); inverted(); segmented(); large_pages()
