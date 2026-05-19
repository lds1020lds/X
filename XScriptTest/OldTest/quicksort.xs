function quicksort(arr, left, right)
{
    if (left < right)
    {
        var pivotIndex = partition(arr, left, right);
        quicksort(arr, left, pivotIndex - 1);
        quicksort(arr, pivotIndex + 1, right);
    }
}

function partition(arr, left, right)
{
    var pivot = arr[right];
    var i = left - 1;
    
    for (j = left; j < right; j++)
    {
        if (arr[j] <= pivot)
        {
            i++;
            swap(arr, i, j);
        }
    }
    
    swap(arr, i + 1, right);
    return i + 1;
}

function swap(arr, i, j)
{
    var temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

function test_quicksort()
{
    // 测试快速排序
    var testArray = [64, 34, 25, 12, 22, 11, 90, 5, 77, 30];
    
    printf("原始数组：");
    print_array(testArray);
    
    quicksort(testArray, 0, testArray:size() - 1);
    
    printf("排序后数组：");
    print_array(testArray);
    
    // 验证排序结果
    var sorted = true;
    for (i = 1; i < testArray:size(); i++)
    {
        if (testArray[i] < testArray[i-1])
        {
            sorted = false;
            break;
        }
    }
    
    if (sorted)
    {
        printf("排序验证：成功！");
    }
    else
    {
        printf("排序验证：失败！");
    }
}

function print_array(arr)
{
    var result = "[";
    for (i = 0; i < arr:size(); i++)
    {
        result = result $ arr[i];
        if (i < arr:size() - 1)
        {
            result = result $ ", ";
        }
    }
    result = result $ "]";
    printf(result);
}

// 主函数
function main()
{
    printf("开始测试快速排序算法...");
    test_quicksort();
    printf("测试完成！");
    system_pause();
}

// 执行主函数
main();