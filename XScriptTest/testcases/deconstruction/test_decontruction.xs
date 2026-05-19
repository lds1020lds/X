function assertEqual(a, b, msg)
{
    if (a != b)
    {
        printf("FAIL", msg, "expected", b, "got", a);
    }
    else
    {
        printf("OK", msg);
    }
}

function makeComplexData()
{
    return {
        id = 1001,
        name = "hero",
        pos = {
            x = 10,
            y = 20,
            z = 30
        },
        stats = [
            { name = "hp", value = 100 },
            { name = "mp", value = 50 },
            { name = "atk", value = 15 }
        ],
        bag = [
            [1, 2, 3],
            { item = "sword", count = 1 },
            [
                { item = "potion", count = 5 },
                { item = "coin", count = 99 }
            ]
        ],
        config = {
            flags = [true, false, true],
            meta = {
                level = 9,
                tags = ["a", "b", "c"]
            }
        },
        nilValue = nil
    };
}

function test_list_basic()
{
    printf("--- test_list_basic ---");

    var [a, b, c] = [1, 2, 3];

    assertEqual(a, 1, "list basic a");
    assertEqual(b, 2, "list basic b");
    assertEqual(c, 3, "list basic c");
}

function test_list_nested()
{
    printf("--- test_list_nested ---");

    var [a, [b, c], [[d], e]] = [1, [2, 3], [[4], 5]];

    assertEqual(a, 1, "nested list a");
    assertEqual(b, 2, "nested list b");
    assertEqual(c, 3, "nested list c");
    assertEqual(d, 4, "nested list d");
    assertEqual(e, 5, "nested list e");
}

function test_table_basic()
{
    printf("--- test_table_basic ---");

    var {id, name} = {id = 100, name = "abc"};

    assertEqual(id, 100, "table id");
    assertEqual(name, "abc", "table name");
}

function test_table_rename()
{
    printf("--- test_table_rename ---");

    // 如果你还没支持 {x: px} 这种重命名语法，可以先注释这个测试。
    var {x: px, y: py} = {x = 11, y = 22};

    assertEqual(px, 11, "rename px");
    assertEqual(py, 22, "rename py");
}

function test_table_nested()
{
    printf("--- test_table_nested ---");

    var data = makeComplexData();

    var {
        id,
        name,
        pos: {x, y, z},
        config: {
            flags: [flag0, flag1, flag2],
            meta: {
                level,
                tags: [tag0, tag1, tag2]
            }
        }
    } = data;

    assertEqual(id, 1001, "nested table id");
    assertEqual(name, "hero", "nested table name");
    assertEqual(x, 10, "pos x");
    assertEqual(y, 20, "pos y");
    assertEqual(z, 30, "pos z");
    assertEqual(flag0, true, "flag0");
    assertEqual(flag1, false, "flag1");
    assertEqual(flag2, true, "flag2");
    assertEqual(level, 9, "level");
    assertEqual(tag0, "a", "tag0");
    assertEqual(tag1, "b", "tag1");
    assertEqual(tag2, "c", "tag2");
}

function test_list_table_mix()
{
    printf("--- test_list_table_mix ---");

    var data = makeComplexData();
    var {
        stats: [
            {name: stat0Name, value: stat0Value},
            {name: stat1Name, value: stat1Value},
            {name: stat2Name, value: stat2Value}
        ],
        bag: [
            [bag00, bag01, bag02],
            {item: swordName, count: swordCount},
            [
                {item: potionName, count: potionCount},
                {item: coinName, count: coinCount}
            ]
        ]
    } = data;

    assertEqual(stat0Name, "hp", "stat0 name");
    assertEqual(stat0Value, 100, "stat0 value");
    assertEqual(stat1Name, "mp", "stat1 name");
    assertEqual(stat1Value, 50, "stat1 value");
    assertEqual(stat2Name, "atk", "stat2 name");
    assertEqual(stat2Value, 15, "stat2 value");

    assertEqual(bag00, 1, "bag00");
    assertEqual(bag01, 2, "bag01");
    assertEqual(bag02, 3, "bag02");
    assertEqual(swordName, "sword", "sword name");
    assertEqual(swordCount, 1, "sword count");
    assertEqual(potionName, "potion", "potion name");
    assertEqual(potionCount, 5, "potion count");
    assertEqual(coinName, "coin", "coin name");
    assertEqual(coinCount, 99, "coin count");
}

var gCallCount = 0;

function makeOnce()
{
    gCallCount++;
    return [10, {x = 20, y = [30, 40]}];
}

function test_rhs_eval_once()
{
    printf("--- test_rhs_eval_once ---");

    var [a, {x, y: [b, c]}] = makeOnce();

    assertEqual(gCallCount, 1, "rhs evaluated once");
    assertEqual(a, 10, "eval once a");
    assertEqual(x, 20, "eval once x");
    assertEqual(b, 30, "eval once b");
    assertEqual(c, 40, "eval once c");
}

function test_missing_and_nil()
{
    printf("--- test_missing_and_nil ---");

    var [a, b, c] = [1];
    assertEqual(a, 1, "missing list a");
    assertEqual(b, nil, "missing list b nil");
    assertEqual(c, nil, "missing list c nil");

    var {x, y, nilValue} = {x = 10, nilValue = nil};
    assertEqual(x, 10, "missing table x");
    assertEqual(y, nil, "missing table y nil");
    assertEqual(nilValue, nil, "explicit nil field");
}

function test_destructure_from_function_return()
{
    printf("--- test_destructure_from_function_return ---");

    var {pos: {x, y}, bag: [[first]]} = makeComplexData();

    assertEqual(x, 10, "function return x");
    assertEqual(y, 20, "function return y");
    assertEqual(first, 1, "function return first bag value");
}

function test_rename_basic()
{
    printf("--- test_rename_basic ---");

    var obj = {x = 10, y = 20, name = "hero"};

    var {x: px, y: py, name: actorName} = obj;

    assertEqual(px, 10, "rename basic px");
    assertEqual(py, 20, "rename basic py");
    assertEqual(actorName, "hero", "rename basic actorName");
}

function test_rename_nested_table()
{
    printf("--- test_rename_nested_table ---");

    var obj = {
        pos = {
            x = 1,
            y = 2,
            z = 3
        },
        info = {
            id = 100,
            title = "npc"
        }
    };

    var {
        pos: {x: px, y: py, z: pz},
        info: {id: entityId, title: entityTitle}
    } = obj;

    assertEqual(px, 1, "rename nested px");
    assertEqual(py, 2, "rename nested py");
    assertEqual(pz, 3, "rename nested pz");
    assertEqual(entityId, 100, "rename nested entityId");
    assertEqual(entityTitle, "npc", "rename nested entityTitle");
}

function test_rename_list_table_mix()
{
    printf("--- test_rename_list_table_mix ---");

    var data = {
        items = [
            {name = "sword", count = 1},
            {name = "potion", count = 5},
            {name = "coin", count = 99}
        ]
    };

    var {
        items: [
            {name: item0Name, count: item0Count},
            {name: item1Name, count: item1Count},
            {name: item2Name, count: item2Count}
        ]
    } = data;

    assertEqual(item0Name, "sword", "rename mix item0 name");
    assertEqual(item0Count, 1, "rename mix item0 count");
    assertEqual(item1Name, "potion", "rename mix item1 name");
    assertEqual(item1Count, 5, "rename mix item1 count");
    assertEqual(item2Name, "coin", "rename mix item2 name");
    assertEqual(item2Count, 99, "rename mix item2 count");
}

function test_rename_same_field_multiple_times()
{
    printf("--- test_rename_same_field_multiple_times ---");

    var obj = {value = 123};

    var {value: a, value: b} = obj;

    assertEqual(a, 123, "rename same field a");
    assertEqual(b, 123, "rename same field b");
}

function test_rename_missing_field()
{
    printf("--- test_rename_missing_field ---");

    var obj = {x = 1};

    var {x: px, y: py} = obj;

    assertEqual(px, 1, "rename missing px");
    assertEqual(py, nil, "rename missing py nil");
}

function test_rename_rhs_expression_once()
{
    printf("--- test_rename_rhs_expression_once ---");

    var callCount = 0;

    function makeObj()
    {
        callCount++;
        return {
            user = {
                id = 7,
                name = "tom"
            }
        };
    }

    var {user: {id: userId, name: userName}} = makeObj();

    assertEqual(callCount, 1, "rename rhs expression once");
    assertEqual(userId, 7, "rename rhs userId");
    assertEqual(userName, "tom", "rename rhs userName");
}

function test_default_list_basic()
{
    printf("--- test_default_list_basic ---");

    var [a = 10, b = 20, c = 30] = [1];

    assertEqual(a, 1, "default list keeps existing a");
    assertEqual(b, 20, "default list missing b");
    assertEqual(c, 30, "default list missing c");
}

function test_default_list_nil_only()
{
    printf("--- test_default_list_nil_only ---");

    var [a = 10, b = 20, c = 30, d = 40] = [nil, 0, false, "value"];

    assertEqual(a, 10, "default list explicit nil");
    assertEqual(b, 0, "default list keeps zero");
    assertEqual(c, false, "default list keeps false");
    assertEqual(d, "value", "default list keeps string");
}

function test_default_table_basic()
{
    printf("--- test_default_table_basic ---");

    var {x = 10, y = 20, z = 30, n = 40} = {x = 1, z = nil};

    assertEqual(x, 1, "default table keeps existing x");
    assertEqual(y, 20, "default table missing y");
    assertEqual(z, 30, "default table explicit nil z");
    assertEqual(n, 40, "default table missing n");
}

function test_default_table_false_and_zero()
{
    printf("--- test_default_table_false_and_zero ---");

    var {enabled = true, count = 99, name = "fallback"} = {enabled = false, count = 0, name = ""};

    assertEqual(enabled, false, "default table keeps false");
    assertEqual(count, 0, "default table keeps zero");
    assertEqual(name, "", "default table keeps empty string");
}

function test_default_rename_basic()
{
    printf("--- test_default_rename_basic ---");

    var obj = {x = 1, y = nil};

    var {x: px = 10, y: py = 20, z: pz = 30} = obj;

    assertEqual(px, 1, "default rename keeps px");
    assertEqual(py, 20, "default rename explicit nil py");
    assertEqual(pz, 30, "default rename missing pz");
}

function test_default_nested_fields()
{
    printf("--- test_default_nested_fields ---");

    var data = {
        pos = {
            x = nil,
            z = 3
        },
        flags = [true]
    };

    var {
        pos: {x = 10, y = 20, z = 30},
        flags: [flag0 = false, flag1 = true, flag2 = false]
    } = data;

    assertEqual(x, 10, "default nested field x");
    assertEqual(y, 20, "default nested field y");
    assertEqual(z, 3, "default nested keeps z");
    assertEqual(flag0, true, "default nested list keeps flag0");
    assertEqual(flag1, true, "default nested list missing flag1");
    assertEqual(flag2, false, "default nested list missing flag2");
}

function test_default_nested_container()
{
    printf("--- test_default_nested_container ---");

    var data = {
        pos = nil,
        bag = nil
    };

    var {
        pos: {x = 1, y = 2} = {x = 10},
        bag: [{item: firstItem = "potion", count: firstCount = 5}] = [{item = "coin"}]
    } = data;

    assertEqual(x, 10, "default nested container table x");
    assertEqual(y, 2, "default nested container table y");
    assertEqual(firstItem, "coin", "default nested container list item");
    assertEqual(firstCount, 5, "default nested container list count");
}

function test_default_list_table_mix()
{
    printf("--- test_default_list_table_mix ---");

    var data = [
        {name = "sword"},
        nil,
        {count = 99}
    ];

    var [
        {name: item0Name = "unknown", count: item0Count = 1},
        {name: item1Name = "potion", count: item1Count = 5} = {},
        {name: item2Name = "coin", count: item2Count = 10}
    ] = data;

    assertEqual(item0Name, "sword", "default mix item0 name");
    assertEqual(item0Count, 1, "default mix item0 count");
    assertEqual(item1Name, "potion", "default mix item1 name");
    assertEqual(item1Count, 5, "default mix item1 count");
    assertEqual(item2Name, "coin", "default mix item2 name");
    assertEqual(item2Count, 99, "default mix item2 count");
}

function test_default_expression_eval_only_when_needed()
{
    printf("--- test_default_expression_eval_only_when_needed ---");

    var callCount = 0;

    function nextDefault(value)
    {
        callCount++;
        return value;
    }

    var [a = nextDefault(10), b = nextDefault(20), c = nextDefault(30)] = [1, nil];

    assertEqual(callCount, 2, "default expression list eval count");
    assertEqual(a, 1, "default expression keeps existing a");
    assertEqual(b, 20, "default expression nil b");
    assertEqual(c, 30, "default expression missing c");
}

function test_default_table_expression_eval_only_when_needed()
{
    printf("--- test_default_table_expression_eval_only_when_needed ---");

    var callCount = 0;

    function makeDefault(value)
    {
        callCount++;
        return value;
    }

    var {x = makeDefault(10), y = makeDefault(20), z = makeDefault(30)} = {x = 1, y = nil};

    assertEqual(callCount, 2, "default expression table eval count");
    assertEqual(x, 1, "default expression table keeps x");
    assertEqual(y, 20, "default expression table nil y");
    assertEqual(z, 30, "default expression table missing z");
}

function test_default_rhs_eval_once()
{
    printf("--- test_default_rhs_eval_once ---");

    var callCount = 0;
    var defaultCount = 0;

    function makeData()
    {
        callCount++;
        return {items = [nil, {value = nil}]};
    }

    function makeValue(value)
    {
        defaultCount++;
        return value;
    }

    var {items: [a = makeValue(10), {value: b = makeValue(20)}]} = makeData();

    assertEqual(callCount, 1, "default rhs eval once data");
    assertEqual(defaultCount, 2, "default rhs eval once defaults");
    assertEqual(a, 10, "default rhs eval once a");
    assertEqual(b, 20, "default rhs eval once b");
}

function test_default_deep_nested_inventory_profile()
{
    printf("--- test_default_deep_nested_inventory_profile ---");

    var data = {
        player = {
            profile = nil,
            inventory = [
                {
                    slot = 0,
                    item = {
                        id = nil,
                        attrs = {
                            power = 12,
                            tags = ["sharp"]
                        }
                    }
                },
                nil,
                {
                    item = {
                        attrs = {
                            tags = [nil, "rare"]
                        }
                    }
                }
            ]
        }
    };

    var {
        player: {
            profile: {name: playerName = "guest", level: playerLevel = 1} = {name = "fallback"},
            inventory: [
                {
                    slot: slot0 = 99,
                    item: {
                        id: item0Id = 1000,
                        attrs: {
                            power: item0Power = 1,
                            durability: item0Durability = 50,
                            tags: [item0Tag0 = "plain", item0Tag1 = "common"]
                        }
                    } = {attrs = {}}
                },
                {
                    slot: slot1 = 1,
                    item: {id: item1Id = 2000, attrs: {power: item1Power = 2} = {}} = {}
                } = {},
                {
                    slot: slot2 = 2,
                    item: {
                        id: item2Id = 3000,
                        attrs: {
                            power: item2Power = 3,
                            tags: [item2Tag0 = "plain", item2Tag1 = "common", item2Tag2 = "normal"]
                        }
                    } = {}
                }
            ]
        }
    } = data;

    assertEqual(playerName, "fallback", "deep profile default container name");
    assertEqual(playerLevel, 1, "deep profile default field level");
    assertEqual(slot0, 0, "deep inventory slot0 existing");
    assertEqual(item0Id, 1000, "deep inventory item0 nil id default");
    assertEqual(item0Power, 12, "deep inventory item0 existing power");
    assertEqual(item0Durability, 50, "deep inventory item0 missing durability");
    assertEqual(item0Tag0, "sharp", "deep inventory item0 existing tag0");
    assertEqual(item0Tag1, "common", "deep inventory item0 missing tag1");
    assertEqual(slot1, 1, "deep inventory item1 default container slot");
    assertEqual(item1Id, 2000, "deep inventory item1 nested default id");
    assertEqual(item1Power, 2, "deep inventory item1 nested default power");
    assertEqual(slot2, 2, "deep inventory slot2 missing slot");
    assertEqual(item2Id, 3000, "deep inventory item2 missing id");
    assertEqual(item2Power, 3, "deep inventory item2 missing power");
    assertEqual(item2Tag0, "plain", "deep inventory item2 nil tag0 default");
    assertEqual(item2Tag1, "rare", "deep inventory item2 existing tag1");
    assertEqual(item2Tag2, "normal", "deep inventory item2 missing tag2");
}

function test_default_deep_nested_matrix_table_mix()
{
    printf("--- test_default_deep_nested_matrix_table_mix ---");

    var data = {
        matrix = [
            [
                {value = 1},
                {extra = 20}
            ],
            nil,
            [
                nil,
                {value = nil, extra = 33}
            ]
        ]
    };

    var {
        matrix: [
            [
                {value: a00 = 100, extra: e00 = 10},
                {value: a01 = 101, extra: e01 = 11},
                {value: a02 = 102, extra: e02 = 12} = {}
            ],
            [
                {value: a10 = 110, extra: e10 = 13},
                {value: a11 = 111, extra: e11 = 14}
            ] = [{}, {}],
            [
                {value: a20 = 120, extra: e20 = 15} = {},
                {value: a21 = 121, extra: e21 = 16}
            ]
        ]
    } = data;

    assertEqual(a00, 1, "deep matrix a00 existing");
    assertEqual(e00, 10, "deep matrix e00 missing");
    assertEqual(a01, 101, "deep matrix a01 missing");
    assertEqual(e01, 20, "deep matrix e01 existing");
    assertEqual(a02, 102, "deep matrix a02 default container");
    assertEqual(e02, 12, "deep matrix e02 default container");
    assertEqual(a10, 110, "deep matrix row1 default a10");
    assertEqual(e10, 13, "deep matrix row1 default e10");
    assertEqual(a11, 111, "deep matrix row1 default a11");
    assertEqual(e11, 14, "deep matrix row1 default e11");
    assertEqual(a20, 120, "deep matrix row2 nil cell a20");
    assertEqual(e20, 15, "deep matrix row2 nil cell e20");
    assertEqual(a21, 121, "deep matrix row2 nil field a21");
    assertEqual(e21, 33, "deep matrix row2 existing e21");
}

function test_default_deep_nested_expression_order()
{
    printf("--- test_default_deep_nested_expression_order ---");

    var log = [];

    function fallback(name, value)
    {
        log:append(name);
        return value;
    }

    var data = {
        root = {
            first = {value = 1},
            second = nil,
            third = {items = [nil, {value = 3}]}
        }
    };

    var {
        root: {
            first: {value: firstValue = fallback("firstValue", 10)} = fallback("firstContainer", {}),
            second: {value: secondValue = fallback("secondValue", 20)} = fallback("secondContainer", {}),
            third: {
                items: [
                    {value: thirdItem0 = fallback("thirdItem0", 30)} = fallback("thirdContainer0", {}),
                    {value: thirdItem1 = fallback("thirdItem1", 40)} = fallback("thirdContainer1", {})
                ] = fallback("items", [])
            } = fallback("third", {})
        }
    } = data;

    assertEqual(firstValue, 1, "deep expression keeps first value");
    assertEqual(secondValue, 20, "deep expression second default value");
    assertEqual(thirdItem0, 30, "deep expression third item0 default value");
    assertEqual(thirdItem1, 3, "deep expression keeps third item1");
    assertEqual(log[0], "secondContainer", "deep expression log second container");
    assertEqual(log[1], "secondValue", "deep expression log second value");
    assertEqual(log[2], "thirdContainer0", "deep expression log third container0");
    assertEqual(log[3], "thirdItem0", "deep expression log third item0");
}

function test_default_deep_nested_repeated_keys()
{
    printf("--- test_default_deep_nested_repeated_keys ---");

    var data = {
        node = {
            id = nil,
            child = {
                id = 2,
                child = nil
            }
        }
    };

    var {
        node: {
            id: rootId = 1,
            id: duplicateRootId = 10,
            child: {
                id: childId = 20,
                child: {
                    id: grandChildId = 30,
                    name: grandChildName = "leaf"
                } = {name = nil}
            } = {}
        }
    } = data;

    assertEqual(rootId, 1, "deep repeated root id default");
    assertEqual(duplicateRootId, 10, "deep repeated duplicate root id default");
    assertEqual(childId, 2, "deep repeated child id existing");
    assertEqual(grandChildId, 30, "deep repeated grand child id default");
    assertEqual(grandChildName, "leaf", "deep repeated grand child nil name default");
}

function test_default_deep_nested_with_skips()
{
    printf("--- test_default_deep_nested_with_skips ---");

    var data = [
        "skip0",
        {
            group = [
                "skip1",
                {value = nil},
                nil
            ]
        }
    ];

    var [,
        {
            group: [,
                {value: selectedValue = 7, label: selectedLabel = "selected"},
                {value: fallbackValue = 8, label: fallbackLabel = "fallback"} = {}
            ]
        }
    ] = data;

    assertEqual(selectedValue, 7, "deep skips nil selected value default");
    assertEqual(selectedLabel, "selected", "deep skips missing selected label");
    assertEqual(fallbackValue, 8, "deep skips default container value");
    assertEqual(fallbackLabel, "fallback", "deep skips default container label");
}


function run_all()
{
    var a, b = 10, 12
    var log = [];
    defer log:append("a")
    defer 
    {
        printf("this is a defer block2 ", a, b);
    }

    defer printf("this is a defer block3");

    test_list_basic();
    test_list_nested();
    test_table_basic();
    test_table_rename();
    test_table_nested();
    test_list_table_mix();
    test_rhs_eval_once();
    test_missing_and_nil();
    test_destructure_from_function_return();

    test_rename_basic()
    test_rename_list_table_mix()
    test_rename_missing_field()
    test_rename_nested_table()
    test_rename_rhs_expression_once()
    test_rename_same_field_multiple_times()

    test_default_list_basic()
    test_default_list_nil_only()
    test_default_table_basic()
    test_default_table_false_and_zero()
    test_default_rename_basic()
    test_default_nested_fields()
    test_default_nested_container()
    test_default_list_table_mix()
    test_default_expression_eval_only_when_needed()
    test_default_table_expression_eval_only_when_needed()
    test_default_rhs_eval_once()
    test_default_deep_nested_inventory_profile()
    test_default_deep_nested_matrix_table_mix()
    test_default_deep_nested_expression_order()
    test_default_deep_nested_repeated_keys()
    test_default_deep_nested_with_skips()
    printf("--- destructure tests done ---");
}

run_all();