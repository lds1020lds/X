// VSCode 调试器测试脚本
// 建议断点位置：
// 1. main() 里的 player 创建后
// 2. update_player(player, 3) 内部循环
// 3. build_nested_data() 返回前
// 4. closure_counter() 内部闭包

function build_nested_data()
{
    var root = {
        name = "root",
        version = 1,
        config = {
            enabled = true,
            retry = 3,
            paths = ["res", "scripts", "logs"]
        }
    };

    root.items = [];
    for (var i = 0; i < 5; i++)
    {
        var item = {
            id = i,
            name = "item_" $ i,
            stats = {
                hp = 100 + i * 10,
                mp = 50 + i * 5
            },
            tags = ["debug", "vscode", "item"]
        };
        root.items:append(item);
    }

    root.lookup = {};
    root.lookup.first = root.items[0];
    root.lookup.last = root.items[list.size(root.items) - 1];

    return root;
}

function make_player(name)
{
    var player = {
        name = name,
        level = 1,
        exp = 0,
        inventory = [],
        attributes = {
            strength = 10,
            agility = 8,
            intelligence = 6
        },
        position = {
            x = 10,
            y = 20,
            z = 0
        }
    };

    player.inventory:append({slot = 0, item = "sword", count = 1});
    player.inventory:append({slot = 1, item = "potion", count = 5});
    player.inventory:append({slot = 2, item = "key", count = 1});

    return player;
}

function update_player(player, rounds)
{
    var totalGain = 0;
    var localHistory = [];

    for (var i = 0; i < rounds; i++)
    {
        var gain = (i + 1) * 15;
        totalGain += gain;
        player.exp += gain;
        player.position.x += i;
        player.position.y += i * 2;

        var snapshot = {
            round = i,
            gain = gain,
            exp = player.exp,
            pos = {
                x = player.position.x,
                y = player.position.y
            }
        };
        localHistory:append(snapshot);
    }

    if (player.exp >= 45)
    {
        player.level += 1;
    }

    return totalGain, localHistory;
}

function closure_counter(start)
{
    var value = start;
    var records = [];

    return function(delta)
    {
        value += delta;
        records:append({delta = delta, value = value});
        return value, records;
    };
}

function inspect_values(player, data, history)
{
    var firstItem = data.lookup.first;
    var lastItem = data.lookup.last;
    var firstHistory = history[0];
    var lastHistory = history[list.size(history) - 1];

    var summary = {
        playerName = player.name,
        playerLevel = player.level,
        inventoryCount = list.size(player.inventory),
        firstItemName = firstItem.name,
        lastItemHp = lastItem.stats.hp,
        firstGain = firstHistory.gain,
        lastExp = lastHistory.exp
    };

    return summary;
}

function main()
{
    printf("--- debug_vscode_test ---");

    var nestedData = build_nested_data();
    var player = make_player("hero");

    var totalGain, history = update_player(player, 3);
    var counter = closure_counter(10);

    var c1, records1 = counter(5);
    var c2, records2 = counter(7);
    var c3, records3 = counter(-2);

    var summary = inspect_values(player, nestedData, history);

    var localsForDebugger = {
        numberValue = 123,
        floatValue = 3.14,
        stringValue = "hello debugger",
        nilValue = nil,
        boolLikeValue = true,
        nestedData = nestedData,
        player = player,
        history = history,
        counterRecords = records3,
        summary = summary
    };

    printf("player=" $ player.name $ ", level=" $ player.level $ ", exp=" $ player.exp);
    printf("totalGain=" $ totalGain $ ", counter=" $ c3);
    printf("summary first=" $ summary.firstItemName $ ", lastHp=" $ summary.lastItemHp);

    return localsForDebugger;
}

main();
