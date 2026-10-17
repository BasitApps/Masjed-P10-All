Import("env")
# access to global construction environment
# print(env)
# access to project construction environment
# print(projenv)
#my_flags = env.ParseFlags(env['BUILD_FLAGS'])
#defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}
# print(defines)
#env.Replace(PROGNAME="firmware_test%s" % defines.get("VERSION"))
names = {
    "p10_1x1_release": "updateP101",
    "p10_2x1_release": "updateP102",
    "p10_3x1_release": "updateP103",
    "p10_4x1_release": "updateP104",
    "p10_4x1": "updateP104_debug",
}
env.Replace(PROGNAME=names[env.subst("$PIOENV")])
