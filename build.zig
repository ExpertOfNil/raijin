const std = @import("std");

const SOLID_SHADER = @embedFile("assets/shaders/solid_shader.wgsl");
const EDGES_SHADER = @embedFile("assets/shaders/edges_shader.wgsl");

const C_FLAGS: []const []const u8 = &.{
    "-std=c11",
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-Wno-gnu-zero-variadic-macro-arguments",
    "-DCGLM_FORCE_DEPTH_ZERO_TO_ONE",
};

const INCLUDE_DIRS: []const []const u8 = &.{
    "include",
    "lib/wgpu/include",
    "lib/cglm/include",
    "lib/cimpl/include",
};

pub fn build(b: *std.Build) void {
    b.resolveInstallPrefix(b.pathFromRoot("."), .{});
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const embedded_shaders = generateEmbeddedShaders(b);

    generateCompileFlags(b);

    addRaijinExe(b, target, optimize, "example-windowed", "examples/windowed.c", embedded_shaders);
    addRaijinExe(b, target, optimize, "example-headless", "examples/headless.c", embedded_shaders);
}

fn addRaijinExe(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    name: []const u8,
    src: []const u8,
    embedded_shaders: std.Build.LazyPath
) void {
    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });

    mod.addCSourceFile(.{
        .file = b.path(src),
        .flags = C_FLAGS,
    });
    mod.addCSourceFile(.{
        .file = embedded_shaders,
        .flags = C_FLAGS,
    });

    for (INCLUDE_DIRS) |dir| {
        mod.addIncludePath(b.path(dir));
    }

    mod.addObjectFile(b.path("lib/wgpu/libwgpu_native.a"));
    mod.addObjectFile(b.path("lib/cglm/libcglm.a"));

    mod.linkSystemLibrary("m", .{});
    mod.linkSystemLibrary("dl", .{});
    mod.linkSystemLibrary("SDL3", .{});
    mod.linkSystemLibrary("unwind", .{});

    const exe = b.addExecutable(.{
        .name = name,
        .root_module = mod,
    });

    b.installArtifact(exe);
}

fn generateCompileFlags(b: *std.Build) void {
    var buf: std.ArrayList(u8) = .empty;

    for (C_FLAGS) |flag| {
        buf.appendSlice(b.allocator, flag) catch @panic("OOM");
        buf.append(b.allocator, '\n') catch @panic("OOM");
    }

    for (INCLUDE_DIRS) |dir| {
        buf.appendSlice(b.allocator, "-I") catch @panic("OOM");
        buf.appendSlice(b.allocator, dir) catch @panic("OOM");
        buf.append(b.allocator, '\n') catch @panic("OOM");
    }

    const update = b.addUpdateSourceFiles();
    update.addBytesToSource(
        buf.toOwnedSlice(b.allocator) catch @panic("OOM"),
        "compile_flags.txt",
    );

    const step = b.step("clangd", "Generate compile_flags.txt for clangd");
    step.dependOn(&update.step);
}

fn appendEmbeddedShader(
    b: *std.Build,
    buf: *std.ArrayList(u8),
    name: []const u8,
    bytes: []const u8,
) void {
    buf.appendSlice(b.allocator, "const unsigned char ") catch @panic("OOM");
    buf.appendSlice(b.allocator, name) catch @panic("OOM");
    buf.appendSlice(b.allocator, "[] = {") catch @panic("OOM");

    for (bytes) |byte| {
        buf.appendSlice(
            b.allocator,
            b.fmt("{d},", .{byte}),
        ) catch @panic("OOM");
    }

    buf.appendSlice(b.allocator, "0};\nconst size_t ") catch @panic("OOM");
    buf.appendSlice(b.allocator, name) catch @panic("OOM");
    buf.appendSlice(b.allocator, "_size = sizeof(") catch @panic("OOM");
    buf.appendSlice(b.allocator, name) catch @panic("OOM");
    buf.appendSlice(b.allocator, ") - 1;\n\n") catch @panic("OOM");
}

fn generateEmbeddedShaders(b: *std.Build) std.Build.LazyPath {
    var buf: std.ArrayList(u8) = .empty;

    buf.appendSlice(b.allocator, "#include <stddef.h>\n\n") catch @panic("OOM");

    appendEmbeddedShader(
        b,
        &buf,
        "raijin_solid_shader_wgsl",
        SOLID_SHADER,
    );

    appendEmbeddedShader(
        b,
        &buf,
        "raijin_edges_shader_wgsl",
        EDGES_SHADER,
    );

    const generated_files = b.addWriteFiles();
    return generated_files.add(
        "raijin_embedded_shaders.c",
        buf.toOwnedSlice(b.allocator) catch @panic("OOM"),
    );
}
