"""
RU: Поведение зонда, на которое опираются навыки: кадры по расписанию, вызов через указатель,
    участок известной длины, блок RTT.
EN: Probe behaviour the skills rely on: scheduled frames, the pointer call, the known-length section,
    the RTT block.
"""
from stm32_gdbtest import case, within


# Consecutive frames are numbered without gaps and are one frame period apart in SysTick ticks.
@case("HW_FRAMES", labels=("probe",))
def frames(t):
    schedule = t.profile.get("user.schedule")
    t.reach("probe_emit_frame")
    first = t.read("probe_telemetry", fields=("frames", "tick"))
    t.reach("probe_emit_frame")
    second = t.read("probe_telemetry", fields=("frames", "tick"))
    t.check("frame numbers are consecutive", second["frames"], first["frames"] + 1)
    period, tolerance = schedule["frame_period_ms"], schedule["tick_tolerance"]
    interval = within(period - tolerance, period + tolerance)
    t.check("frames are one period apart", second["tick"] - first["tick"], interval)
    t.record("frames", {"first": first, "second": second})


# probe_sample() is reached through the pointer in RAM (the target of the RTT pointer variant).
@case("HW_SAMPLE_HOOK", labels=("probe", "rtt"))
def sample_hook(t):
    t.check("the hook points to probe_sample", t.evaluate("probe_sample_hook == &probe_sample"), 1)
    t.reach("probe_sample")
    before = t.evaluate("probe_telemetry.samples")
    t.reach("probe_sample")
    t.check("each call counts a sample", t.evaluate("probe_telemetry.samples"), (before + 1) & 0xFFFF)
    result = t.finish()
    t.check("probe_sample returns to its caller", result["outcome"], "completed")


# probe_compute() returns a value: the target of the return-value (lr) variant of stm32-rtt-console.
@case("HW_COMPUTE_RETURN", labels=("probe", "rtt"))
def compute_return(t):
    t.reach("probe_compute")
    argument = t.evaluate("input")
    result = t.finish()
    expected = (argument * 2654435761 + 1) & 0xFFFFFFFF
    expected ^= (expected << 13) & 0xFFFFFFFF
    expected ^= expected >> 17
    expected ^= (expected << 5) & 0xFFFFFFFF
    t.check("probe_compute returns the xorshift of its argument", result["return_value"], expected)


# The known-length section runs the configured number of iterations and DWT counts its cycles.
@case("HW_WORK", labels=("probe",))
def work(t):
    schedule = t.profile.get("user.schedule")
    t.reach("probe_work")
    t.check("iterations from the schedule", t.evaluate("iterations"), schedule["work_iterations"])
    result = t.finish()
    cycles = result["return_value"]

    # Each iteration is at least one nop and a branch; the bound only rejects a stopped counter.
    t.check("DWT counted the section", cycles, within(schedule["work_iterations"], schedule["work_iterations"] * 20))
    t.check("telemetry keeps the cycles", t.evaluate("probe_telemetry.work_cycles"), cycles)
    t.record("work", {"iterations": schedule["work_iterations"], "cycles": cycles})


# The RTT control block is complete and its buffers are in RAM, so OpenOCD can find it.
@case("HW_RTT_BLOCK", labels=("probe", "rtt"))
def rtt_block(t):
    rtt = t.profile.get("user.rtt")
    board = t.profile.data["board"]["board"]
    t.reach("probe_loop")
    block = t.symbol("_SEGGER_RTT")
    identifier = bytes(t.memory(block["address"], 16)).split(b"\0", 1)[0].decode("ascii")
    t.check("RTT identifier", identifier, rtt["id"])
    buffers = t.read("_SEGGER_RTT", fields=("max_up", "max_down"))
    t.check("one up and one down buffer", buffers, {"max_up": 1, "max_down": 1})
    t.check("up buffer size", t.evaluate("_SEGGER_RTT.up[0].size"), rtt["up_size"])
    t.check("down buffer size", t.evaluate("_SEGGER_RTT.down[0].size"), rtt["down_size"])
    ram = within(0x20000000, 0x20000000 + board["ram_kib"] * 1024 - 1)
    t.check("up buffer in RAM", t.evaluate("(unsigned int)_SEGGER_RTT.up[0].buffer"), ram)
    t.check("boot log reached the up buffer", t.evaluate("_SEGGER_RTT.up[0].write_offset") > 0)
