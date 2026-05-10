#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
unsafe fn panic(_: &PanicInfo) -> ! {
  extern "C" {
    pub fn abort() -> !;
  }
  abort();
}

#[no_mangle]
pub fn add(a: u32, b: u32) -> u32 {
  a + b
}