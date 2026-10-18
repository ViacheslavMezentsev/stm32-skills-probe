"""
RU: Запуск зонда: нет отказа, тактирование из файла данных платы, профиль прогона совпадает с платой.
EN: Probe startup: no fault, the clock from the board data, the run profile matches the board.
"""
from stm32_gdbtest import case, one_of, within

SRAM_START = 0x20000000


# The probe reaches its main loop on the crystal or, when the crystal did not start, on HSI.
@case("HW_BOOT", labels=("boot",))
def boot(t):
    clock = t.profile.data["board"]["clock"]
    t.reach("probe_loop")
    t.check([
        ("no platform fault", "platform_fault", 0),
        ("no unexpected exception", "probe_fault_vector", 0),
        ("probe_state set after the boot log", "probe_state", 0xA5),
    ])
    core_hz = t.evaluate("SystemCoreClock")
    t.check("core clock is HSE or HSI", core_hz, one_of(clock["hse_hz"], clock["hsi_hz"]))
    # Flags bit 0 is HSE, bit 1 is an HSE start failure: exactly one source is reported.
    flags = t.evaluate("probe_telemetry.flags")
    t.check("telemetry flags agree with the clock", flags & 1, int(core_hz == clock["hse_hz"] and (flags & 2) == 0))
    t.record("clock", {"core_hz": core_hz, "flags": flags})


# The image, the board data and the build belong to the board the run configuration names.
@case("HW_BOARD_PROFILE", labels=("boot", "profile"))
def board_profile(t):
    profile = t.profile
    board = profile.data["board"]["board"]

    t.check(f"{profile['mcu']} is a {board['name']}", board["name"] in profile["mcu"])
    t.check(f"the build defines {board['device']}", board["device"] in profile.build["defines"])
    t.check("the board data comes from its file", profile.origin("data.board.board.name")["file"],
            profile.files["data.board"]["reference"])

    identity = profile["identity"]
    chip = t.evaluate(f"*(volatile unsigned int *)0x{identity['address']:08X}") & identity["mask"]
    t.check(f"DEV_ID 0x{chip:03X} matches {profile['mcu']}", chip, identity["value"])

    vectors = t.memory(profile["flash_start"], 8)
    stack, reset = int.from_bytes(vectors[:4], "little"), int.from_bytes(vectors[4:], "little")
    flash = within(profile["flash_start"], profile["flash_start"] + profile["flash_size"] - 1)
    t.check("initial stack pointer at the end of SRAM", stack, SRAM_START + board["ram_kib"] * 1024)
    t.check("reset vector in the profile flash", reset & ~1, flash)
    t.check("reset vector is Thumb code", reset & 1)

    # A peripheral register is not read as memory: a read could change the device state.
    uart = profile.data["board"]["uart"]
    with t.refused("outside_window", name="USART1 data register is refused as memory"):
        t.memory(uart["data_register"], 4)

    t.check("stand backend", profile.stand["backend"], one_of("openocd", "jlink", "stlink"))
    t.record("profile", profile)
