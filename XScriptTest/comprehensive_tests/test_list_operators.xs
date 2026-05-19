// ============================================================
// 测试 list 的 + 和 * 运算符
// ============================================================

printf("========================================");
printf("  测试 list 的 + 和 * 运算符");
printf("========================================");

// 测试 list + list（拼接）
printf("--- 测试 list + list ---");


var list1 = [1, 2, 3];
var list2 = [4, 5, 6];
var result1 = list1 + list2;
var si = list1:size()
var t = { 1, 5, s = "sss", b = "bbb" }
t = t $ 1111
t *= 2
printf(f"list1 = {list1}");
printf(f"list2 = {list2}");
printf("list1 + list2 = ");
for (var i = 0; i < list.size(result1); i = i + 1) {
    printf(f"  result1[{i}] = {result1[i]}");
}

// 验证原列表未被修改
printf(f"原 list1 长度 = {list.size(list1)} (应为 3)");
printf(f"原 list2 长度 = {list.size(list2)} (应为 3)");

// 测试 list + element（追加元素）
printf("--- 测试 list + element ---");
var list3 = ["a", "b"];
var result2 = list3 + "c";
printf(f"list3 = {list3}");
printf("list3 + 'c' = ");
for (var i = 0; i < list.size(result2); i = i + 1) {
    printf(f"  result2[{i}] = {result2[i]}");
}

// 测试 list * int（重复列表）
printf("--- 测试 list * int ---");
var list4 = [1, 2];
var result3 = list4 * 3;
printf(f"list4 = {list4}");
printf("list4 * 3 = ");
for (var i = 0; i < list.size(result3); i = i + 1) {
    printf(f"  result3[{i}] = {result3[i]}");
}

// 测试 list * 0
printf("--- 测试 list * 0 ---");
var result4 = list4 * 0;
printf(f"list4 * 0 长度 = {list.size(result4)} (应为 0)");

// 测试 list * 1
printf("--- 测试 list * 1 ---");
var result5 = list4 * 1;
printf(f"list4 * 1 长度 = {list.size(result5)} (应为 2)");
for (var i = 0; i < list.size(result5); i = i + 1) {
    printf(f"  result5[{i}] = {result5[i]}");
}

// 测试 list * 负数（应该返回空列表）
printf("--- 测试 list * 负数 ---");
var result6 = list4 * -1;
printf(f"list4 * -1 长度 = {list.size(result6)} (应为 0)");

// 测试空列表的运算
printf("--- 测试空列表运算 ---");
var empty = [];
var result7 = empty + [1, 2, 3];
printf(f"[] + [1, 2, 3] 长度 = {list.size(result7)} (应为 3)");
var result8 = empty * 5;
printf(f"[] * 5 长度 = {list.size(result8)} (应为 0)");

printf("========================================");
printf("  list + 和 * 运算符测试完成");
printf("========================================");
