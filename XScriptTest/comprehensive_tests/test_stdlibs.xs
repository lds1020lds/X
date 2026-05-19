// ============================================================
// XScript 综合测试 - 新标准库
// 覆盖：json/path/time/table/regex/http
// ============================================================

function test_json_lib()
{
    printf("--- test_json_lib ---");

    var obj = json.decode("{\"name\":\"xscript\",\"count\":3,\"items\":[1,2,null],\"nested\":{\"ok\":true}}");
    assertEqual(obj.name, "xscript", "json.decode string field");
    assertEqual(obj.count, 3, "json.decode number field");
    assertEqual(list.size(obj.items), 3, "json.decode array size");
    assertEqual(obj.items[0], 1, "json.decode array value");
    assertEqual(obj.items[2], nil, "json.decode null value");
    assertEqual(obj.nested.ok, 1, "json.decode true value");

    var source = {title = "hello", num = 12, child = {value = "ok"}};
    var encoded = json.encode(source);
    var decoded = json.decode(encoded);
    assertEqual(decoded.title, "hello", "json.encode/decode string");
    assertEqual(decoded.num, 12, "json.encode/decode int");
    assertEqual(decoded.child.value, "ok", "json.encode/decode nested table");

    var arr = [1, "two", nil];
    var arr2 = json.decode(json.encode(arr));
    assertEqual(list.size(arr2), 3, "json.encode/decode list size");
    assertEqual(arr2[1], "two", "json.encode/decode list value");
    assertEqual(arr2[2], nil, "json.encode/decode list nil");
}

function test_path_lib()
{
    printf("--- test_path_lib ---");

    assertEqual(path.join("a", "b", "..", "c.txt"), "a/c.txt", "path.join normalize");
    assertEqual(path.normalize("a//b/./c"), "a/b/c", "path.normalize");
    assertEqual(path.basename("a/b/c.txt"), "c.txt", "path.basename");
    assertEqual(path.dirname("a/b/c.txt"), "a/b", "path.dirname");
    assertEqual(path.extname("a/b/c.txt"), ".txt", "path.extname");
    assertEqual(path.isabs("C:/tmp/a.txt"), 1, "path.isabs drive");
    assert(type(path.abspath(".")) == "string", "path.abspath returns string");
    assertEqual(path.relative("a/b", "a/c/d"), "../c/d", "path.relative sibling");
}

function test_time_lib()
{
    printf("--- test_time_lib ---");

    var now = time.now();
    assert(type(now) == "int", "time.now returns int");
    assert(time.clock() >= 0, "time.clock non-negative");

    var t = time.localtime(0);
    assertEqual(t.year, 1970, "time.localtime year shape");
    assert(t.month >= 1 && t.month <= 12, "time.localtime month range");
    assert(type(time.format("%Y", 0)) == "string", "time.format returns string");

    var u = time.utc(0);
    assertEqual(u.year, 1970, "time.utc year");
}

function test_table_lib()
{
    printf("--- test_table_lib ---");

    var t = {a = 1, b = 2};
    assertEqual(table.size(t), 2, "table.size");
    assertEqual(table.contains(t, "a"), 1, "table.contains existing");
    assertEqual(table.contains(t, "missing"), 0, "table.contains missing");

    var keys = table.keys(t);
    var values = table.values(t);
    assertEqual(list.size(keys), 2, "table.keys size");
    assertEqual(list.size(values), 2, "table.values size");

    var shallow = table.clone(t);
    assertEqual(shallow.a, 1, "table.clone shallow");

    var deepSrc = {child = {value = 3}};
    var deepClone = table.clone(deepSrc, 1);
    deepClone.child.value = 4;
    assertEqual(deepSrc.child.value, 3, "table.clone deep isolates nested table");

    var dst = {a = 1};
    table.merge(dst, {b = 2});
    assertEqual(dst.b, 2, "table.merge");

    table.clear(dst);
    assertEqual(table.size(dst), 0, "table.clear");
}

function test_regex_lib()
{
    printf("--- test_regex_lib ---");

    assertEqual(regex.regmatch("^abc$", "abc"), 1, "regex.match true");
    assertEqual(regex.regmatch("^abc$", "ab"), 0, "regex.match false");

    var matches = regex.search("a([0-9]+)", "a12 b a34");
    assert(matches != nil, "regex.search returns matches");
    assert(list.size(matches) >= 1, "regex.search match count");
    assertEqual(matches[0].content, "a12", "regex.search first content");
}

function test_http_lib()
{
    printf("--- test_http_lib ---");

    var result, err = http.request("GET", "not-a-valid-url");
    assertEqual(result, nil, "http.request invalid url returns nil");
    assert(type(err) == "int", "http.request invalid url returns error code");
}
