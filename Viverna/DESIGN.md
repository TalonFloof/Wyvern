# Anchor Sector
```forge
type {
    pub VivernaAnchor = packed struct {
        magic: u32, // "VIVA"
        reserved: u32,
        first_group_lba: u64
    };
}
```

The Anchor Sector is used to point to the first aligned allocation group. This is used to ensure that the entire filesystem uses allocation groups that are perfectly aligned with the preferred erase size of the NAND flash. If the partition is already aligned, then no Anchor Sector will be written.

# Superblock
```forge
type {
    pub VivernaSuperblock = packed struct {
        magic: u32, // "VIVS"
        
    };
}
```