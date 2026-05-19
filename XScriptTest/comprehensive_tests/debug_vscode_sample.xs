// ============================================================
// VSCode XScript 调试器测试脚本
// 用途：测试断点、单步、调用栈、局部变量、全局变量、表/list 展开
// 建议断点位置：
// 1. main() 开头
// 2. buildPlayer() 内部
// 3. computeDamage() 内部
// 4. for 循环内部
// 5. 闭包 counter() 内部
// ============================================================

printLine = function(title)
{
    printf("---- " $ title $ " ----");
};

globalSeed = 7;
globalConfig = {
    difficulty = "normal",
    baseDamage = 10,
    enableCritical = true
};

function buildPlayer(name, level)
{
    var player = {
        name = name,
        level = level,
        hp = 100 + level * 20,
        mp = 50 + level * 5,
        inventory = ["sword", "potion"]
    };

    var testShadow = 100
    {

        var testShadow  = 10000
        printf(testShadow)
        printf(player.level)
    }

    player.stats = {
        strength = 10 + level,
        agility = 8 + level / 2,
        defense = 5 + level
    };

    player.takeDamage = function(self, damage)
    {
        var oldHp = self.hp;
        self.hp -= damage;
        if (self.hp < 0)
        {
            self.hp = 0;
        }
        return oldHp - self.hp;
    };

    return player;
}

function computeDamage(attacker, target, skillPower)
{
    var base = globalConfig.baseDamage + attacker.stats.strength;
    var scaled = base * skillPower;
    var reduced = scaled - target.stats.defense;
    var critical = 0;

    if (globalConfig.enableCritical && attacker.level > target.level)
    {
        critical = attacker.level - target.level;
        reduced += critical;
    }

    if (reduced < 1)
    {
        reduced = 1;
    }

    return reduced, critical;
}

function makeCounter(prefix)
{
    var count = 0;
    return function(delta)
    {
        count += delta;
        var label = prefix $ count;
        return count, label;
    };
}

function runBattleRound(roundIndex, player, monster)
{
    var skillPower = roundIndex + 1;
    var damage, critical = computeDamage(player, monster, skillPower);
    var realDamage = monster:takeDamage(damage);

    var roundInfo = {
        round = roundIndex,
        skillPower = skillPower,
        damage = damage,
        critical = critical,
        realDamage = realDamage,
        monsterHp = monster.hp
    };

    printf("round=" $ roundIndex $ ", damage=" $ realDamage $ ", monsterHp=" $ monster.hp);
    return roundInfo;
}

function main()
{
    printLine("debug sample start");

    var player = buildPlayer("hero", 5);
    var monster = buildPlayer("slime", 2);
    monster.hp = 180;
    monster.stats.defense = 3;

    var history = [];
    for (var i = 0; i < 4; i++)
    {
        var info = runBattleRound(i, player, monster);
        history:append(info);

        if (monster.hp <= 0)
        {
            break;
        }
    }

    var a = 0;
    var b = 1;
    for(var i = 0; i < 100; i++)
    {
        a = a + i;
        b += 1;
    }


    var counter = makeCounter("step-");
    var c1, label1 = counter(1);
    var c2, label2 = counter(2);
    var c3, label3 = counter(3);

    var summary = {
        playerName = player.name,
        monsterName = monster.name,
        playerHp = player.hp,
        monsterHp = monster.hp,
        rounds = list.size(history),
        counterValue = c3,
        counterLabel = label3
    };

    printf("summary rounds=" $ summary.rounds $ ", counter=" $ summary.counterLabel);
    printLine("debug sample end");

    return summary;
}

var result = main();
printf("final monster hp=" $ result.monsterHp);
